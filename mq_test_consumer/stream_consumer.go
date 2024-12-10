package main

import (
	"context"
	"fmt"
	"sync"
	"time"

	"github.com/confluentinc/confluent-kafka-go/kafka"
	"go.uber.org/zap"
)

type FetchResult struct {
	msg *kafka.Message
	sn  string
	cid string
}

type StreamConsumer interface {
	GetId() int
	GetConsTopic() string
	GetProdTopic() string
	Process(fr *FetchResult) bool
}

func Run(sc StreamConsumer, sugar *zap.SugaredLogger, ctx context.Context, wg *sync.WaitGroup, cc *kafka.ConfigMap, pc kafka.ConfigMap, intv, tmcnt int) {

	defer wg.Done()

	// create consumer
	c, err := kafka.NewConsumer(cc)

	if err != nil {
		sugar.Panicf("creating consumer failed, id : %d, err : %s", sc.GetId(), err)
	}

	defer c.Close()

	// create producer
	tid, _ := pc.Get("transactional.id", "")
	ntid := fmt.Sprintf("%v", tid) + string(sc.GetId())
	pc.SetKey("transactional.id", ntid)

	p, err := kafka.NewProducer(&pc)

	if err != nil {
		sugar.Panicf("creating producer failed, id : %d, err : %s", sc.GetId(), err)
	}

	err = p.InitTransactions(context.Background())
	if err != nil {
		sugar.Panicf("initializing producer transaction failed, id : %d, err : %s",
			sc.GetId(), err.Error())
	}

	defer p.Close()

	// subscribe consumer topic
	err = c.SubscribeTopics([]string{sc.GetConsTopic()}, nil)

	if err != nil {
		sugar.Panicf("subscribing topic failed, id : %d, topic : %s, err : %s",
			sc.GetId(), sc.GetConsTopic(), err.Error())
	}

	sugar.Infof("client message consumer %d started for topic : %s, tid : %s",
		sc.GetId(), sc.GetConsTopic(), ntid)

	var cnt = 0
	var rcnt = 0
	var trans = false
	var lfr *FetchResult = nil

	for {
		select {
		case <-ctx.Done():
			sugar.Infof("[consumer %d] stopping...", sc.GetId())
			return
		default:

			// fetch message
			fr := Fetch(sc, sugar, c, intv)
			if fr.msg == nil {
				if trans && lfr != nil {
					if !Commit(sc, sugar, c, p, lfr) {
						p.AbortTransaction(context.Background())
						sugar.Panicf("[consumer %d] cannot commit transaction, cid : %s, sn : %s",
							sc.GetId(), lfr.cid, lfr.sn)
					}
					trans = false
					lfr = nil
					cnt = 0
				}
				continue
			}
			lfr = fr

			// process message
			for !sc.Process(fr) {
				// retry
				time.Sleep(time.Duration(intv) * time.Millisecond)

				rcnt++
				if rcnt > 100000 { // 100 sec
					sugar.Panicf("[consumer %d] cannot process msg, cid : %s, sn : %s",
						sc.GetId(), fr.cid, fr.sn)
				}
			}
			rcnt = 0

			ptp := sc.GetProdTopic()
			if ptp == "" {
				// commit offset

				continue
			}

			// transaction
			if !trans {
				if !BeginTran(sc, sugar, c, p) {
					sugar.Panicf("[consumer %d] cannot begin transaction, cid : %s, sn : %s",
						sc.GetId(), fr.cid, fr.sn)
				}
				trans = true
				cnt = 0
				rcnt = 0
			}

			// produce message
			for !Produce(sc, sugar, c, p, fr) {
				// retry
				time.Sleep(time.Duration(intv) * time.Millisecond)

				rcnt++
				if rcnt > 100000 { // 100 sec
					sugar.Panicf("[consumer %d] cannot produce message, cid : %s, sn : %s",
						sc.GetId(), fr.cid, fr.sn)
				}
			}
			cnt++
			rcnt = 0

			if cnt%tmcnt == 0 && trans {
				if !Commit(sc, sugar, c, p, fr) {
					p.AbortTransaction(context.Background())
					sugar.Panicf("[consumer %d] cannot commit transaction, cid : %s, sn : %s",
						sc.GetId(), fr.cid, fr.sn)
				}
				trans = false
				cnt = 0
			}
		}
	}
}

func Fetch(sc StreamConsumer, sugar *zap.SugaredLogger, c *kafka.Consumer, intv int) *FetchResult {

	msg, err := c.ReadMessage(time.Duration(intv) * time.Millisecond)
	if err == nil {
		var sn, cid string
		for _, header := range msg.Headers {
			if header.Key == "sn" {
				sn = string(header.Value)
			} else if header.Key == "cid" {
				cid = string(header.Value)
			}
		}

		sugar.Infof("[consumer %d] topic : %s, timestamp : %s, cid : %s, sn : %s, msg : %s",
			sc.GetId(), msg.TopicPartition, msg.Timestamp.Format(time.RFC3339), cid, sn, string(msg.Value))

		return &FetchResult{msg, sn, cid}

	} else if err.(kafka.Error).Code() == kafka.ErrTimedOut {
		// nothing
	} else {
		sugar.Errorf("[consumer %d] fetch failed, err : %s", err.Error())
	}

	return &FetchResult{nil, "", ""}
}

func BeginTran(sc StreamConsumer, sugar *zap.SugaredLogger, c *kafka.Consumer, p *kafka.Producer) bool {

	err := p.BeginTransaction()
	if err != nil {
		sugar.Errorf("[consumer %d] beginning transaction failed, ctp : %s, err : %s",
			sc.GetId(), sc.GetConsTopic(), err.Error())
		return false
	}

	return true
}

func Produce(sc StreamConsumer, sugar *zap.SugaredLogger, c *kafka.Consumer, p *kafka.Producer, fr *FetchResult) bool {

	ptp := sc.GetProdTopic()
	err := p.Produce(&kafka.Message{
		TopicPartition: kafka.TopicPartition{
			Topic: &ptp, Partition: kafka.PartitionAny},
		Value: fr.msg.Value,
		Headers: []kafka.Header{
			{Key: "sn", Value: []byte(fr.sn)},
			{Key: "cid", Value: []byte(fr.cid)},
		},
	}, nil)
	if err != nil {
		sugar.Errorf("[consumer %d] producing message failed, ptp : %s, sn : %s, cid : %s, err : %s",
			sc.GetId(), sc.GetProdTopic(), fr.sn, fr.cid, err.Error())

		return false
	}

	return true
}

func Commit(sc StreamConsumer, sugar *zap.SugaredLogger, c *kafka.Consumer, p *kafka.Producer, fr *FetchResult) bool {

	cgm, err := c.GetConsumerGroupMetadata()
	if err != nil {
		sugar.Errorf("[consumer %d] getting consumer group metadata failed, ctp : %s, sn : %s, cid : %s, err : %s",
			sc.GetId(), sc.GetConsTopic(), fr.sn, fr.cid, err.Error())
		return false
	}

	err = p.SendOffsetsToTransaction(context.Background(),
		[]kafka.TopicPartition{fr.msg.TopicPartition}, cgm)
	if err != nil {
		sugar.Errorf("[consumer %d] committing offset failed, ctp : %s, sn : %s, cid : %s, err : %s",
			sc.GetId(), sc.GetConsTopic(), fr.sn, fr.cid, err.Error())
		return false
	}

	err = p.CommitTransaction(context.Background())
	if err != nil {
		sugar.Errorf("[consumer %d] committing transaction failed, ctp : %s, sn : %s, cid : %s, err : %s",
			sc.GetId(), sc.GetConsTopic(), fr.sn, fr.cid, err.Error())
		return false
	}

	sugar.Infof("[consumer %d] commit succeeded, cid : %s, sn : %s",
		sc.GetId(), fr.cid, fr.sn)
	return true
}
