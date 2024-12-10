package main

import (
	"context"
	"flag"
	"net/url"
	"os"
	"os/signal"
	"sync"
	"syscall"

	"github.com/confluentinc/confluent-kafka-go/kafka"
	"github.com/redis/go-redis/v9"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

func main() {

	mqs := flag.String("mqs", "localhost:29092,localhost:39092,localhost:49092", "mq server to connect")
	group := flag.String("group", "mq_test_consumer", "mq group id to consume")
	ctp := flag.String("ctp", "", "consumer topic")
	cnt := flag.Int("cnt", 3, "consumer count")
	intv := flag.Int("intv", 100, "consumer polling interval(ms)")
	ptp := flag.String("ptp", "", "producer topic")
	tid := flag.String("tid", "", "transactional id prefix for producer")
	mode := flag.String("mode", "cmc", "consumer mode (default: client message counter)")
	rsi := flag.String("rsi", "redis://default:bitnami@localhost:7000/", "mq server to connect")
	tmcnt := flag.Int("tmcnt", 100, "transaction message count for bulk")

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

	logger, _ := config.Build()
	defer logger.Sync()
	sugar := logger.Sugar()

	if *ctp == "" {
		sugar.Error("no consumer topic entered")
		flag.Usage()
		os.Exit(1)
	}

	if *ptp != "" && *tid == "" {
		sugar.Error("transactional id should be entered for producer")
		flag.Usage()
		os.Exit(1)
	}

	if *ptp == "" {
		sugar.Warn("no work for producer")
	}

	signalChan := make(chan os.Signal, 1)
	signal.Notify(signalChan, os.Interrupt, syscall.SIGTERM)

	// kafka
	consCnf := kafka.ConfigMap{
		"bootstrap.servers":  *mqs,
		"group.id":           *group,
		"auto.offset.reset":  "earliest",
		"enable.auto.commit": false,
		"isolation.level":    "read_committed",
	}

	prodCnf := kafka.ConfigMap{
		"bootstrap.servers":  *mqs,
		"transactional.id":   *tid, // producer id will be appended at start
		"enable.idempotence": true,
	}

	// redis
	ru, err := url.Parse(*rsi)
	if err != nil {
		sugar.Error("invalid redis server uri")
		flag.Usage()
		os.Exit(1)
	}
	rp, _ := ru.User.Password()
	opt := redis.ClusterOptions{
		Addrs:    []string{ru.Host},
		Password: rp,
	}

	rc := redis.NewClusterClient(&opt)

	ctx, cancel := context.WithCancel(context.Background())
	var wg sync.WaitGroup

	// run stream consumers
	for i := 0; i < *cnt; i++ {

		wg.Add(1)

		var sc StreamConsumer
		if *mode == "cmc" {
			sc = ClntMsgCnter{sugar, i, *ctp, *ptp, rc}
		} else {

		}

		pcnf := make(kafka.ConfigMap)
		for key, value := range prodCnf {
			pcnf[key] = value
		}
		go Run(sc, sugar, ctx, &wg, &consCnf, pcnf, *intv, *tmcnt)
	}

	sugar.Info("press ctrl+c to stop")
	<-signalChan

	sugar.Warn("signal received, shutting down...")
	cancel()

	wg.Wait()
	sugar.Info("all consumers and producers stopped, exiting...")
}
