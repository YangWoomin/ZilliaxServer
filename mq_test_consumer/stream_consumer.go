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
	Process(fr *FetchResult, ig *bool) bool
}

type Pendings map[int32]kafka.Offset
type Partitions map[int32]kafka.TopicPartition

func Run(sc StreamConsumer, sugar *zap.SugaredLogger, ctx context.Context, wg *sync.WaitGroup, cc *kafka.ConfigMap, pc kafka.ConfigMap, intv, tmcnt int) {

	defer wg.Done()

	var maxRcnt int = 3000
	var txtimeout int = 30 // 30 sec

	// create consumer
	c, err := kafka.NewConsumer(cc)
	if err != nil {
		sugar.Fatalf("creating consumer failed, id : %d, err : %s", sc.GetId(), err)
	}

	defer c.Close()

	// create producer
	var p *kafka.Producer = CreateProducer(sc, sugar, &pc, txtimeout)
	if p != nil {
		defer p.Close()
		defer func() {
			ctx, cancel := context.WithTimeout(context.Background(), time.Duration(txtimeout)*time.Second)
			defer cancel()

			if err := p.AbortTransaction(ctx); err != nil {
				sugar.Warnf("[consumer %d] AbortTransaction failed, err : %s",
					sc.GetId(), err.Error())
			}
		}()
	}

	var cnt = 0
	pts := make(Partitions)
	pendings := make(Pendings)

	// reference : https://github.com/confluentinc/confluent-kafka-go/blob/bb108e529f91/examples/consumer_rebalance_example/consumer_rebalance_example.go
	rbc := func(cns *kafka.Consumer, event kafka.Event) error {
		switch e := event.(type) {
		case kafka.AssignedPartitions:
			sugar.Infof("[consumer %d] Assigned Partitions: %v",
				sc.GetId(), e.Partitions)
			cns.Assign(e.Partitions)

		case kafka.RevokedPartitions:
			sugar.Infof("[consumer %d] Revoked Partitions: %v",
				sc.GetId(), e.Partitions)

			if cnt > 0 && p != nil {
				ctx, cancel := context.WithTimeout(context.Background(), time.Duration(txtimeout)*time.Second)
				defer cancel()

				sugar.Infof("[consumer %d] transaction rollback because of rebalance, %v",
					sc.GetId(), e.Partitions)

				p.AbortTransaction(ctx)
			}
			pendings = make(Pendings)
			cnt = 0

			// remove the partitions to be unowned
			for _, p := range e.Partitions {
				delete(pts, p.Partition)
			}

			// rollback the owning partitions offset to consume again
			for _, p := range pts {
				err := cns.Seek(p, txtimeout*1000)
				if err != nil && err.(kafka.Error).Code() != kafka.ErrNoOffset {
					sugar.Panicf("[consumer %d] Seeking parttiona failed in rebalance, topic : %s, partition : %d",
						sc.GetId(), *p.Topic, p.Partition)
				}
			}

			pts = make(Partitions)
		}
		return nil
	}

	// subscribe consumer topic
	err = c.SubscribeTopics([]string{sc.GetConsTopic()}, rbc)
	if err != nil {
		sugar.Fatalf("subscribing topic failed, id : %d, topic : %s, err : %s",
			sc.GetId(), sc.GetConsTopic(), err.Error())
	}

	sugar.Infof("client message consumer %d started for topic : %s",
		sc.GetId(), sc.GetConsTopic())

	var rcnt = 0

	for {
		select {
		case <-ctx.Done():
			sugar.Infof("[consumer %d] stopping...", sc.GetId())
			return
		default:

			// fetch message
			fr := Fetch(sc, sugar, c, intv)
			if fr.msg == nil {
				if cnt > 0 {
					if !CommitTx(sc, sugar, c, p, pendings, txtimeout) {
						sugar.Errorf("[consumer %d] cannot commit transaction, %v",
							sc.GetId(), pendings)
					} else {
						pendings = make(Pendings)
						pts = make(Partitions)
						cnt = 0
					}
				}
				continue
			}

			// process message
			ig := false
			for !sc.Process(fr, &ig) && !ig {
				// retry
				time.Sleep(time.Duration(intv) * time.Millisecond)

				rcnt++
				if rcnt > maxRcnt {
					sugar.Errorf("[consumer %d] cannot process msg, cid : %s, sn : %s",
						sc.GetId(), fr.cid, fr.sn)
					return
				}
			}
			rcnt = 0

			// begin transaction
			if cnt == 0 {
				if !BeginTx(sc, sugar, p) {
					sugar.Errorf("[consumer %d] cannot begin transaction, cid : %s, sn : %s",
						sc.GetId(), fr.cid, fr.sn)
					return
				}
			}

			// produce message
			for !ig && !Produce(sc, sugar, c, p, fr) {
				// retry
				time.Sleep(time.Duration(intv) * time.Millisecond)

				rcnt++
				if rcnt > maxRcnt {
					sugar.Errorf("[consumer %d] cannot produce message, cid : %s, sn : %s",
						sc.GetId(), fr.cid, fr.sn)
					return
				}
			}
			rcnt = 0

			if pendings[fr.msg.TopicPartition.Partition] < fr.msg.TopicPartition.Offset+1 {
				pendings[fr.msg.TopicPartition.Partition] = fr.msg.TopicPartition.Offset + 1
			}
			cnt++

			if pts[fr.msg.TopicPartition.Partition].Partition == 0 {
				pts[fr.msg.TopicPartition.Partition] = fr.msg.TopicPartition
			}

			// commit transaction
			if cnt%tmcnt == 0 {
				if !CommitTx(sc, sugar, c, p, pendings, txtimeout) {
					sugar.Errorf("[consumer %d] cannot commit transaction, cid : %s, sn : %s",
						sc.GetId(), fr.cid, fr.sn)
				} else {
					pendings = make(Pendings)
					pts = make(Partitions)
					cnt = 0
				}
			}
		}
	}
}

func CreateProducer(sc StreamConsumer, sugar *zap.SugaredLogger, pc *kafka.ConfigMap, timeout int) *kafka.Producer {
	if sc.GetProdTopic() != "" {
		tid, _ := pc.Get("transactional.id", "")
		ntid := fmt.Sprintf("%v_%d", tid, sc.GetId())
		pc.SetKey("transactional.id", ntid)

		p, err := kafka.NewProducer(pc)
		if err != nil {
			sugar.Panicf("creating producer failed, id : %d, err : %s", sc.GetId(), err)
		}

		ctx, cancel := context.WithTimeout(context.Background(), time.Duration(timeout)*time.Second)
		defer cancel()

		err = p.InitTransactions(ctx)
		if err != nil {
			sugar.Panicf("initializing producer transaction failed, id : %d, err : %s",
				sc.GetId(), err.Error())
		}

		return p
	}
	return nil
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
			sc.GetId(), *msg.TopicPartition.Topic, msg.Timestamp.Format(time.RFC3339), cid, sn, string(msg.Value))

		return &FetchResult{msg, sn, cid}

	} else if err.(kafka.Error).Code() == kafka.ErrTimedOut {
		// nothing
	} else {
		sugar.Errorf("[consumer %d] fetch failed, err : %s", sc.GetId(), err.Error())
	}

	return &FetchResult{nil, "", ""}
}

func BeginTx(sc StreamConsumer, sugar *zap.SugaredLogger, p *kafka.Producer) bool {

	if p == nil {
		return true
	}

	err := p.BeginTransaction()
	if err != nil {
		sugar.Errorf("[consumer %d] beginning transaction failed, ctp : %s, err : %s",
			sc.GetId(), sc.GetConsTopic(), err.Error())
		return false
	}

	return true
}

func Produce(sc StreamConsumer, sugar *zap.SugaredLogger, c *kafka.Consumer, p *kafka.Producer, fr *FetchResult) bool {

	if p == nil {
		return true
	}

	ptp := sc.GetProdTopic()
	err := p.Produce(&kafka.Message{
		TopicPartition: kafka.TopicPartition{
			Topic: &ptp, Partition: kafka.PartitionAny},
		Key:   []byte(fr.cid),
		Value: fr.msg.Value,
		Headers: []kafka.Header{
			{Key: "sn", Value: []byte(fr.sn)},
			{Key: "cid", Value: []byte(fr.cid)},
		},
	}, nil)
	if err != nil {
		sugar.Errorf("[consumer %d] producing message failed, ptp : %s, cid : %s, sn : %s, err : %s",
			sc.GetId(), sc.GetProdTopic(), fr.cid, fr.sn, err.Error())

		return false
	}

	// sugar.Infof("[consumer %d] producing message succeeded, ptp : %s, cid : %s, sn : %s, offset : %d",
	// 	sc.GetId(), sc.GetProdTopic(), fr.cid, fr.sn, fr.msg.TopicPartition.Offset)

	return true
}

func CommitTx(sc StreamConsumer, sugar *zap.SugaredLogger, c *kafka.Consumer, p *kafka.Producer, ps Pendings, timeout int) bool {

	offsets := []kafka.TopicPartition{}
	tp := sc.GetConsTopic()

	for key, value := range ps {
		offsets = append(offsets, kafka.TopicPartition{
			Topic:     &tp,
			Partition: key,
			Offset:    value,
		})
	}

	if p == nil {
		_, err := c.CommitOffsets(offsets)
		if err != nil {
			sugar.Errorf("[consumer %d] committing offset failed, ctp : %s, err : %s, ps : %v",
				sc.GetId(), sc.GetConsTopic(), err.Error(), ps)
			return false
		}

		return true
	}

	ctx, cancel := context.WithTimeout(context.Background(), time.Duration(timeout)*time.Second)
	defer cancel()

	cgm, err := c.GetConsumerGroupMetadata()
	if err != nil {
		sugar.Errorf("[consumer %d] getting consumer group metadata failed, ctp : %s, err : %s, ps : %v",
			sc.GetId(), sc.GetConsTopic(), err.Error(), ps)
		return false
	}

	err = p.SendOffsetsToTransaction(ctx, offsets, cgm)
	if err != nil {
		sugar.Errorf("[consumer %d] committing offset failed, ctp : %s, err : %s, ps : %v",
			sc.GetId(), sc.GetConsTopic(), err.Error(), ps)
		return false
	}

	err = p.CommitTransaction(ctx)
	if err != nil {
		sugar.Errorf("[consumer %d] committing transaction failed, ctp : %s, err : %s, ps : %v",
			sc.GetId(), sc.GetConsTopic(), err.Error(), ps)
		return false
	}

	// sugar.Infof("[consumer %d] commit succeeded, ps : %v",
	// 	sc.GetId(), ps)
	return true
}
