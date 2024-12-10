package main

import (
	"context"
	"fmt"
	"sync"
	"time"

	"github.com/confluentinc/confluent-kafka-go/kafka"
	"go.uber.org/zap"
)

func ClientMessageConsumer(sugar *zap.SugaredLogger, ctx context.Context, id int, wg *sync.WaitGroup, kc *kafka.ConfigMap, tp string, interval int) {

	defer wg.Done()

	c, err := kafka.NewConsumer(kc)

	if err != nil {
		sugar.Panicf("Creating new consumer failed in client message consumer. id : %d, err : %s", id, err)
	}

	defer c.Close()

	// todo: handling rebalance
	err = c.SubscribeTopics([]string{tp}, nil)

	if err != nil {
		sugar.Panicf("Subscribing topic failed in client message consumer. id : %d", id)
	}

	sugar.Infof("Client message consumer %d started for topic : %s", id, tp)

	for {
		select {
		case <-ctx.Done():
			fmt.Printf("Consumer %d: Stopping...\n", id)
			return
		default:
			msg, err := c.ReadMessage(time.Duration(interval) * time.Millisecond)
			if err == nil {
				var sn string
				for _, header := range msg.Headers {
					if header.Key == "sn" {
						sn = string(header.Value)
						break
					}
				}
				sugar.Infof("[Consumer %d] topic : %s, timestamp : %s, sn : %s, msg : %s",
					id, msg.TopicPartition, msg.Timestamp.Format(time.RFC3339), sn, string(msg.Value))
			} else if err.(kafka.Error).Code() == kafka.ErrTimedOut {
				// nothing
			} else {
				fmt.Printf("Consumer error: %v (%v)\n", err, msg)
			}
		}
	}
}
