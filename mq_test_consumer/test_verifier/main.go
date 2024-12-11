package main

import (
	"context"
	"flag"
	"net/url"
	"os"
	"strconv"

	"github.com/redis/go-redis/v9"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

func main() {

	svr := flag.String("svr", "redis://default:bitnami@localhost", "redis server to connect without port")
	port := flag.Int("port", 7000, "redis server start port")
	cnt := flag.Int("cnt", 3, "redis server cnt")
	key := flag.String("key", "consumer:*:msg_cnt", "key to scan data")
	same := flag.Bool("same", true, "check all results are same")

	flag.Parse()

	// logger
	config := zap.NewDevelopmentConfig()
	config.EncoderConfig = zapcore.EncoderConfig{
		TimeKey:       "ts",
		LevelKey:      "level",
		NameKey:       "logger",
		CallerKey:     "caller",
		MessageKey:    "msg",
		StacktraceKey: "stacktrace",
		EncodeLevel:   zapcore.CapitalColorLevelEncoder,
		EncodeTime:    zapcore.ISO8601TimeEncoder,
		EncodeCaller:  zapcore.ShortCallerEncoder,
	}

	config.Encoding = "console"

	config.OutputPaths = []string{"stdout", "../log/mq_test_verifier.log"}

	logger, _ := config.Build()
	defer logger.Sync()
	sugar := logger.Sugar()

	ctx := context.Background()

	var first bool = true
	var issame bool = true
	var value string
	var rows int = 0
	for i := 0; i < *cnt; i++ {
		p := *port + i
		u := *svr + ":" + strconv.Itoa(p)

		ru, err := url.Parse(u)
		if err != nil {
			sugar.Errorf("invalid redis server uri, %s", u)
			flag.Usage()
			os.Exit(1)
		}
		rp, _ := ru.User.Password()
		opt := redis.Options{
			Addr:     ru.Host,
			Password: rp,
		}
		rc := redis.NewClient(&opt)

		var cursor uint64
		for {
			keys, nextCursor, err := rc.Scan(ctx, cursor, *key, 10).Result()
			if err != nil {
				sugar.Error("scanning keys failed, server : %s, err : %v", ru.Host, err)
				return
			}

			for _, key := range keys {
				val := rc.Get(ctx, key)
				if first {
					value = val.Val()
					first = false
				} else if val.Val() != value && *same {
					sugar.Warnf("mismatched value, key : %s, value : %s, host : %s", key, value, ru.Host)
					issame = false
				}

				if len(key) > 40 {
					key = key[:40] + "..."
				}

				sugar.Infof("key : %s, value : %s", key, value)
				rows++
			}

			cursor = nextCursor
			if cursor == 0 {
				break
			}
		}
	}

	if *same && !issame {
		sugar.Info("all results are not equal")
	} else if *same {
		sugar.Info("all results are equal")
	}

	sugar.Infof("test completed, rows : %d", rows)
}
