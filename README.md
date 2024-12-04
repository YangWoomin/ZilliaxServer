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
* Database module (ODBC connector)
* Asynchronous network module (IOCP, epoll)

## TO-DO Features
* Network module - RIO, io_uring, TLS, HTTP(S) Server
* Server framework - ECS(?)
* Physics system
* NavMesh & Navigation
* AI System (Behavior Tree)
* Memory database module - redis(reids++)
* Message queue client module - kafka(librdkafka)
* Distributed System - inter-server communication
* Containerize
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
* mysql odbc driver : mysql-connector-odbc-9.0.0-win64.msi (in ./setting/database/driver directory)

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
* move cmd current working directory to ./db
* build db module by the following command or vs code task

```bash
make rebuild_debug
```

#### "db_test" Tester Build
* move cmd current working directory to ./db_test
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

#### "network" Module Build
* same way as "db" module
### "network_test" module
* this test program is for testing "network" module
* this program consists of three components - chat server, chat client and chat massive test client
#### "network_test" Tester Build
* same way as "db_test" tester
#### Chat Server
* this is default mode of network_test
* you can choose unicast(default) echo or broadcast echo mode
* printing received messages from chat clients is skipped
* run example

```bash
./network_testd.out --broadcast # you can quit by ctrl + c
```

![image](https://github.com/user-attachments/assets/77daaa69-e68f-4693-a4d9-1bfb179d4f9d)

#### Chat Client
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
### "mq" module
* this module is core of producing and consuming messages to and from message queue(Kafka) asynchronously
* the following features are or would be supported

| Feature | Implemented | Tested |
|----------|----------|----------|
| Producer | ⬜ | ⬜ |
| Consumer | ⬜ | ⬜ |


### Prerequisite
#### Docker & Docker Compose Installation 
* refer to 

#### Kafka and Conduktor on Docker
* move cmd(bash shell) current working directory to ./setting/mq/kafka/docker-compose

```bash
docker-compose up -d
docker-compose ps
```

#### Setting Kafka Cluster on Conduktor
* Conduktor Guide Page : https://docs.conduktor.io/platform/get-started/installation/get-started/docker/
* Add Kafka Cluster like the following picture

![conduktor_adding_kafka_cluster](https://github.com/user-attachments/assets/9f58d118-99a1-4da8-9f53-c7332f933c01)


