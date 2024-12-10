package main

import (
	"context"
	"flag"
	"os"
	"os/signal"
	"sync"
	"syscall"

	"github.com/confluentinc/confluent-kafka-go/kafka"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

func main() {

	server := flag.String("server", "localhost:29092,localhost:39092,localhost:49092", "mq server to connect")
	group := flag.String("group", "mq_test_consumer", "mq group id to consume")
	clientMsgTopic := flag.String("cmTopic", "client_message", "client message topic")
	//msgAggrTopic := flag.String("maTopic", "message_aggregation", "message aggregation topic")
	cmConsumerCount := flag.Int("cmConsumerCount", 3, "client message consumer count")
	interval := flag.Int("consumerPollIntMs", 100, "consumer polling interval(ms)")
	//maConsumerCount := flag.Int("maConsumerCount", 3, "message aggregation consumer count")

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

	signalChan := make(chan os.Signal, 1)
	signal.Notify(signalChan, os.Interrupt, syscall.SIGTERM)

	kConf := kafka.ConfigMap{
		"bootstrap.servers": *server,
		"group.id":          *group,
		"auto.offset.reset": "earliest",
	}

	ctx, cancel := context.WithCancel(context.Background())
	var wg sync.WaitGroup

	// run client message consumer
	for i := 0; i < *cmConsumerCount; i++ {
		wg.Add(1)
		go ClientMessageConsumer(sugar, ctx, i, &wg, &kConf, *clientMsgTopic, *interval)
	}

	// run message aggregation consumer

	sugar.Info("Press Ctrl+C to stop.")
	<-signalChan

	sugar.Warn("Signal received. Shutting down...")
	cancel()

	wg.Wait()
	sugar.Info("All consumers stopped. Exiting.")
}
