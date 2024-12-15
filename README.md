# ZilliaxServer

## Contents Table
* [Purpose](#Purpose)
* [Introduction](#Introduction)
* [Features](#Features)
* [TO-DO Features](#TO-DO-Features)
* [Build Tool and Driver](#Build-Tool-and-Driver)
* [License](#License)
* [Development Environment](#Development-Environment)
* [Build](#Build)
* [Debugging](#Debugging)
* [Database](#Database)
* [Network](#Network)
* [Message Queue](#Message-Queue)
* [Cache](#Cache)

## Purpose
* MMO (RPG) Game Server

## Introduction
![zilliax](https://github.com/user-attachments/assets/ee2cae56-2a7d-4f2c-9895-6adb8c0a3f2b)

> I used to play Hearthstone, and one of my favorite cards from the game is Zilliax. \
> The card's abilities are truly impressive! \
> Inspired by this card, I named my project "ZilliaxServer." \
> Just like Zilliax's "Magnetic" ability, this project aims to help you build your game server by combining with it seamlessly.

## Features
* C/C++(20)
* Cross platform (Windows/Linux)
* Database module (MySQL)
* Asynchronous network module (IOCP, epoll)
* Memory Database module (Redis)
* Message Queue module (Kafka)

## TO-DO Features
* Network module - RIO, io_uring, TLS, HTTP(S) Server
* Server framework - ECS(?)
* Physics system
* NavMesh & Navigation
* AI System (Behavior Tree)
* Containerization
* Google Breakpad

## Build Tool and Driver
* Make/CMake
* MSVC/g++
* ODBC Driver (MySQL)

## License
* ZilliaxServer : Apache 2.0
* spdlog-1.14.1 (https://github.com/gabime/spdlog/tree/v1.14.1) - MIT License
* cxxopts-3.2.0 (https://github.com/jarro2783/cxxopts/tree/v3.2.0) - MIT License
* librdkafka-2.6.1 (https://github.com/confluentinc/librdkafka/tree/v2.6.1) - Apache 2.0
* libuv (https://github.com/libuv/libuv) - MIT License
* hiredis (https://github.com/redis/hiredis) - MIT License
* redis++ (https://github.com/sewenew/redis-plus-plus) - Apache 2.0
* (check more details in ./third-party/readme.txt)

## Development Environment

### IDE
* Visual Studio 2022
* Visual Studio Code

## Build

### Windows Prerequisite
* os : Windows 10 (latest build)
* Visual Studio 2022 (Community)
* make : make-3.8.1.exe (in ./setting/build_tool directory)
* cmake : cmake-3.30.0-windows-x86_64.msi (in ./setting/build_tool directory)
* core utils : coreutils-5.3.0.exe (in ./setting/build_tool directory)
* mysql odbc driver : mysql-connector-odbc-9.0.0-win64.msi (in ./setting/database/mysql/driver directory)

### Linux (Ubuntu 24.04 on wsl2) Prerequisite
* os : Ubuntu 24.04 LTS
* make

```bash
sudo apt install make
```

* cmake

```bash
sudo apt install cmake
```

* gcc/g++

```bash
sudo apt install build-essential
```

* mysql odbc driver

```bash
sudo apt install unixodbc-dev unixodbc
sudo dpkg -i mysql-community-client-plugins_9.0.0-1ubuntu22.04_amd64.deb # (in ./setting/database/driver directory)
sudo apt update
sudo apt install -f
sudo dpkg -i mysql-connector-odbc_9.0.0-1ubuntu22.04_amd64.deb # (in ./setting/database/driver directory)
sudo apt update
sudo apt install -f
```
    
### Build 
#### "x64 Native Tools Command Prompt for VS 2022" on Windows or bash shell on Linux
* configure

```bash
make configure
```

* reconfigure

```bash
make reconfigure
```

* build debug

```bash
make build_debug
```

* rebuild debug

```bash
make rebuild_debug
```

* build release

```bash
make build_release
```

* rebuild release

```bash
make rebuild_release
```

* distclean

```bash
make distclean
```

#### VS Code on Windows
* Ctrl + Shift + B
* select one of the list that you want to build
#### VS Code on Linux (wsl2)
* Open VS Code and Connect to WSL
  + reference : https://code.visualstudio.com/docs/remote/wsl
  + check "Debugging -> VS Code on Linux (wsl2)"
* Ctrl + Shift + B
* select one of the list that you want to build

## Debugging
### VS Code on Windows
* open VS Code
* Ctrl + Shift + D
* select one of the launch list you want to debug
  + ![image](https://github.com/user-attachments/assets/513a55a3-8619-44ee-b983-d39c813ab689)
* click the play icon
### VS Code on Linux (wsl2)
* install gdb    
  + sudo apt install gdb
* VS Code setting for WSL : https://code.visualstudio.com/docs/remote/wsl
* install "WSL" extension of Visual Studio Code
* open VS Code from WSL cmd or connect to WSL from VS Code on Windows
  + ![image](https://github.com/user-attachments/assets/1aedf17e-991a-4675-bfae-eaa0e27e6872)
* Ctrl + Shift + D
* select one of the launch list you want to debug
* click the play icon

## Database
### "db" module
* this module is core of interating with RDBMS such as MySQL and MSSQL by using ODBC Driver
### "db_test" module
* this test program is for testing "db" module by which some test cases from simple manipulation works(select, update, delete, sp) to some transactions run
### Run Environment
* Ubuntu 24.04 on wsl2 + docker + docker-compose + MySQL Workbench

### Prerequisite
#### Docker & Docker Compose Installation 
* docker-ce : https://docs.docker.com/engine/install/ubuntu/
* docker-compose : https://docs.docker.com/compose/install/standalone/#on-linux

```bash
# docker user setting
sudo usermod -aG docker $USER
newgrp docker
```

#### MySQL on Docker
* move cmd(bash shell) current working directory to ./setting/database/mysql/docker-compose

```bash
docker-compose up -d
docker-compose ps
```

### "db" Module Test
#### initial scripts
* run ./setting/database/mysql/scripts/db_test/init.sql at MySQL Workbench 
#### "db" Module Build
* move cmd(or shell) current working directory to ./db
* build db module by the following command or vs code task

```bash
make rebuild_debug
```

#### "db_test" Tester Build
* move cmd(or shell) current working directory to ./db_test
* build db_test by the following command or vs code task

```bash
make rebuild_debug
```

#### Test
* move cmd current working directory to ./output/bin
* run db_test(d).exe or db_test(d).out

```batch
.\db_testd.exe
```

```bash
./db_testd.out
```

* check log files in ./output/log directory
* some cases are testable such as simple select, update, delete, stored procedure, transaction

## Network
### "network" module
* this module is core of sending and receiving data on network synchronously/asynchronously
* the following features are or would be supported

| Feature | Implemented | Tested |
|----------|----------|----------|
| IPv4 | ✅ | ✅ |
| IPv6 | ✅ | ⬜ |
| TCP | ✅ | ✅ |
| UDP | ⬜ | ⬜ |
| Sync | ⬜ | ⬜ |
| Async | ✅ | ✅ |

### "network_test" module
* this test program is for testing "network" module
* this program consists of three components - chat server, chat client and chat massive test client

### "network" Module Test

#### "network" Module Build
* move cmd(or shell) current working directory to ./network
* build network module by the following command or vs code task

```bash
make rebuild_debug
```

#### "network_test" Tester Build
* move cmd(or shell) current working directory to ./network_test
* build db_test by the following command or vs code task

```bash
make rebuild_debug
```

#### Run Chat Server
* this is default mode of network_test
* you can choose unicast(default) echo or broadcast echo mode
* printing received messages from chat clients is skipped
* run example

```bash
./network_testd.out --broadcast # you can quit by ctrl + c
```

![image](https://github.com/user-attachments/assets/77daaa69-e68f-4693-a4d9-1bfb179d4f9d)

#### Run Chat Client
* you can send messages one by one to the chat server on console stdin
* a message received from the chat server is printed on console stdout 
* run example

```bash
./network_testd.out --mode client # you can quit by typing "exit"
```

![image](https://github.com/user-attachments/assets/79d3c13f-d407-453e-882e-73a01e6cbf16)

#### Chat Massive Test Client
* this mode automatically sends text of sample test files in ./network_test/test_sample_files/* to the chat server
* this mode creates a number of connections (default: 100) to the chat server and they send the sample text simultaneously (for making the chat server handle large traffic)
* the text file contents are echoed from the chat server and printed on console stdout (1 per 1000, avoiding for large printing)
* run example

```bash
./network_testd.out --mode mtc # you can quit by ctrl + c
```

![image](https://github.com/user-attachments/assets/0fc8387f-df15-4e1b-b148-89ebfdb48a18)
![image](https://github.com/user-attachments/assets/8bbb5be8-3c1c-4fa3-bd5e-72e6b3a0e32c)

### Additional Explanation and Notes
#### TCP Custom Message Format
* the network module has a TCP custom message format
  + a call for Connection.Send() make a message prefixed by its size
  + TODO) we will also support stream communication on TCP socket
#### Message Size
* the maximum message size is approximately 4K (defined in network/common.h)
#### Domain Name
* you can use domain name as host ip when calling Network::Connect()
  + the module tries to resolve domain name if "host" field is not ip


## Message Queue
### "mq" Module
* this module is core of producing and consuming messages to and from message queue(Kafka) asynchronously
* this module can make our servers send messages or events to other servers asynchronously by using the message queue
* the following features are or would be supported

| Feature | Implemented | Tested |
|----------|----------|----------|
| Producer | ✅ | ✅ |
| Consumer | ⬜ | ⬜ |

* **NOTE**: servers on windows cannot find redis and kafka clusters in wsl2 

### "mq" Module Build
* move cmd(or shell) current working directory to ./mq
* build mq module by the following command or vs code task

```bash
make rebuild_debug
```

* this module contains librdkafkacpp source files in ./mq/src/librdkafkacpp and links librdkafka(C)
* if you want to have a new version of librdkafka check the guide file in ./setting/mq/kafka/librdkafka/install_librdkafka.txt

## Cache
### "cache" Module
* this module is core of data access and manipulation to memory database(Redis) synchronously or asynchronously
* this module can make our servers stateless by saving data in memory database so that those have scalability as cloud native application
* now we have only Lua script run by exposed interface but will support to run simple commands by redis++ native facilities
* the following features are or would also supported

| Feature | Standalone | Cluster |
|----------|----------|----------|
| Sync | ⬜ | ✅ |
| Async | ⬜ | ✅ |

* **NOTE**: servers on windows cannot find redis and kafka clusters in wsl2 

### "cache" Module Build
* move cmd(or shell) current working directory to ./cache
* build cache module by the following command or vs code task

```bash
make rebuild_debug
```

* if you have any problem for redis++ such as compilation, link or run build your own redis++ shared library by referring to ./setting/database/redis/redis++/install_redis++.txt

## Cache & Message Queue Test
* we build and test "network", "network_test", "mq", "cache", "mq_test_producer", and "mq_test_consumer" modules comprehensively in this chapter
* this test scenario aims to count client messages per client and message, and to save the counting results while ensuring data integrity and consistency

### Overview

![zilliax_server_overview](https://github.com/user-attachments/assets/f2ad3955-1b19-42fe-a8ba-cab2e0734b2f)

### Module Relationship
* Client : "network_test"
* Producer Server : "mq_test_producer"
* Cache Server : Redis Cluster, accessed from "cache" module
* Message Queue : Kafka Cluster, accessed from "mq" module
* Client Message Counter : "mq_test_consumer" (Golang)
* Message Aggregator : "mq_test_consumer" (same as Client Message Counter, Golang)

### Producer Server
* this server receives messages from client by "network" module, store temporarily in memory database by "cache" module, and enqueue those in the message queue by "mq" module

#### Build
* move shell current working directory to ./mq_test_producer
* build mq_test_producer module by the following command or vs code task

```bash
make rebuild_debug
```

* this module needs shared libraries built from "network", "mq", and "cache"

### Client Message Counter & Message Aggregator
* these servers consume messages from the message queue
* Client Message consumes messages produced from Producer Server, counts messages per client, and reproduces messages to the message queue for Message Aggregator
* Message Aggregator consumes messages produced from Client Message, counts messages per message
* "mq_test_consumer" module is made by Golang (I need some Golang experience 😜)

#### Install Golang
* Windows : **not supported** (because confluent-kafka-go doesn't support on windows officially)
* Linux (Ubuntu) : ./setting/lan/go1.23.4.linux-amd64.tar.gz

```bash
tar -C /usr/local -xzf go1.23.4.linux-amd64.tar.gz
```

#### Build
* move shell current working directory to ./mq_test_consumer
* build mq_test_consumer module by the following command

```bash
./build.sh
```

* move shell current working directory to ./mq_test_consumer/test_verifier
* build mq_test_verifier by the following command

```bash
./build.sh
```

### Prerequisite
#### Run Environment
* Ubuntu 24.04 on wsl2 + docker + docker-compose

#### Docker & Docker Compose Installation 
* refer to "Database - Prerequisite - Docker & Docker Compose Installation"

#### Kafka Cluster and Conduktor on Docker
* move shell current working directory to ./setting/mq/kafka/docker-compose

```bash
docker-compose up -d
docker-compose ps
```

* there are six brokers that are composed of three controllers(KRaft) and normal brokers respectively

![image](https://github.com/user-attachments/assets/e212c952-8d04-4118-928a-f6f659065c02)

#### Set Kafka Cluster on Conduktor
* http://localhost:8080
* add kafka cluster like the following picture

![conduktor_adding_kafka_cluster](https://github.com/user-attachments/assets/392ca274-00ed-4f91-a93e-d15736b7babb)

* explore a lot of facilities for managing and monitoring kafka cluster in Conduktor

* if you want to remove postgresql fatal messages such as "role "root" does not exist", "database "root" does not exist" run the following commands in the postgresql container
```bash
docker exec -it [your postgresql container id or name] /bin/bash
psql -U conduktor -d conduktor-console
CREATE ROLE root WITH LOGIN PASSWORD 'your_password';
CREATE DATABASE root OWNER root;
```

#### Redis Cluster and Redis Insight on Docker
* move shell current working directory to ./setting/database/redis/docker-compose

```bash
docker-compose up -d
docker-compose ps
```

* there are six redis nodes that are composed of three master nodes and replica nodes respectively

![image](https://github.com/user-attachments/assets/e36c54a1-5441-4351-86d2-ff9140de621c)


#### Set Redis Cluster on Redis Insight
* http://localhost:5540/
* add redis cluster like the following picture

![image](https://github.com/user-attachments/assets/704e16f1-9289-45d5-819b-d4e32b760e30)

* enter "bitnami" as password if you didn't change password in docker-compose file
* explore a lot of facilities for managing and monitoring redis cluster in Redis Insight  

### Test

#### Create "client_message" and "message_aggregation" Topics
* http://localhost:8080/console/my-local-cluster/topics
* click "+ New topic" at right top on the page
* enter "client_message" topic name, change partition count to 10, and click "Create topic" at right bottom on the page

![image](https://github.com/user-attachments/assets/9f833088-637e-465a-90d8-03c334e3e3a2)

* click "+ New topic" again to create "message_aggregation" topic
* enter "message_aggregation" topic name, change partition count to 10, and click "Create topic" at right bottom on the page

#### Run Client Message Counter
* move shell current working directory to ./output/bin

```bash
./mq_test_consumer.out --ctp=client_message --ptp=message_aggregation --tid=tid
```

* pass the parameters ctp that means consumer topic, ptp that means producer topic, and tid that means transactional id
* if you want to know other parameter options check the source file ./mq_test_consumer/main.go

#### Run Message Aggregator
* move shell current working directory to ./output/bin

```bash
./mq_test_consumer.out --group=mq_test_consumer1 --ctp=message_aggregation --mode=ma
```

* pass the arguments group that menas consumer group, ctp that means consumer topic, and ma as mode that means message aggregator
* if you want to know other parameter options check the source file ./mq_test_consumer/main.go

#### Run Producer Server
* move shell current working directory to ./output/bin

```bash
./mq_test_producerd.out
```

* there is no parameter because the server uses default parameter values
* if you want to know other parameter options check the source file ./mq_test_producer/src/main.cpp

#### Run Client
* move shell current working directory to ./output/bin

```bash
./network_testd.out --mode=mtc --num=10
```

* pass the parameters mtc as mode that means massive test client and num that means the number of clients
* if you want to know other parameter options check the source file ./network_test/src/main.cpp

#### Check "client_message" Topic Records

![image](https://github.com/user-attachments/assets/9f770cfd-280e-4473-b538-0e5a7dc447b6)

* there are 44,220 records received from 10 clients each of which sends 4422 messages

#### Check "message_aggregation" Topic Records

![image](https://github.com/user-attachments/assets/42bbf38f-8845-4c8b-9b42-a72f3e18f747)

* there are more than 44,220 records because transaction markers are also included

#### Check Message Count per Client
* move shell current working directory to ./output/bin

```bash
./mq_test_verifier.out --key=consumer:*:msg_cnt
```

![image](https://github.com/user-attachments/assets/d95baaf7-d456-43f2-85cd-c7bac19e1a70)

* check that each client sent 4422 messages that are stored in redis

#### Check Message Count per Message
* move shell current working directory to ./output/bin

```bash
./mq_test_verifier.out --key=message:*:msg_cnt
```

![image](https://github.com/user-attachments/assets/9f376c9c-b97e-486b-b7f6-9eefaf4e3cbf)


* check that each message is stored 10 times from 10 clients and all message count is 4422

