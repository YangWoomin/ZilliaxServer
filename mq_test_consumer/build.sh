#!/bin/bash

GODEBUG=gocache=1 /usr/local/go/bin/go build -v -o ../output/bin/mq_test_consumer.out .
