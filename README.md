# ZilliaxServer

## Purpose
* MMO (RPG) Game Server

## Introduction
![zilliax](https://github.com/user-attachments/assets/ef1c2415-1056-49af-a76b-b7be73e7e7cf)


## Features
* C/C++(20)
* Cross platform (Windows/Linux)
* Asynchronous network module (IOCP, epoll)
* Database module (ODBC connector)

## Dependencies
* Make/CMake
* MSVC/G++
* ODBC Driver (MySQL)

## Development Envrionment

### IDE
* Visual Studio 2022
* Visual Studio Code

## Build

### Windows Prerequisites
* os : Windows 10 (latest build)
* Visual Studio 2022 (Community)
* make : make-3.8.1.exe (in ./setting/build_tool directory)
* cmake : cmake-3.30.0-windows-x86_64.msi (in ./setting/build_tool directory)
* core utils : coreutils-5.3.0.exe (in ./setting/build_tool directory)
* mysql odbc driver : mysql-connector-odbc-9.0.0-win64.msi (in ./setting/database/driver directory)

### Linux (Ubuntu 22.04 on wsl2) Prerequisites
* os : Ubuntu 24.04 LTS
* make : sudo apt install make
* cmake : sudo apt install cmake
* gcc/g++ : sudo apt install build-essential
* mysql odbc driver
  + sudo apt install unixodbc-dev unixodbc
  + sudo dpkg -i mysql-community-client-plugins_9.0.0-1ubuntu22.04_amd64.deb (in ./setting/database/driver directory)
  + sudo apt update
  + sudo apt install -f
  + sudo dpkg -i mysql-connector-odbc_9.0.0-1ubuntu22.04_amd64.deb (in ./setting/database/driver directory)
  + sudo apt update
  + sudo apt install -f
    
### Build ("x64 Native Tools Command Prompt for VS 2022" on Windows or bash shell on Linux)
* configure : make configure
* reconfigure : make reconfigure
* build debug : make build_debug
* rebuild debug : make rebuild_debug
* build release : make build_release
* rebuild debug : make rebuild_release
* distclean : make distclean

## Database
* env : Ubuntu 24.04 on wsl2 + docker + docker-compose + MySQL Workbench

### Docker & Docker Compose Installation 
* docker-ce : https://docs.docker.com/engine/install/ubuntu/
* docker-compose : https://docs.docker.com/compose/install/standalone/#on-linux
* sudo usermod -aG docker $USER 
* newgrp docker

### Run MySQL by Docker
* move cmd(bash shell) current working directory to ./setting/database/mysql
* docker-compose up -d
* docker-compose ps

### Test db module
#### Apply db scripts in database
* run ./setting/database/mysql/scripts/db_test/init.sql by MySQL Workbench 
#### Build db module
* move cmd current working directory to ./db
* build db module by "make distclean && make rebuild_debug"
#### Build db_test
* move cmd current working directory to ./db_test
* build db_test by "make rebuild_debug"
#### Run db_test
* some cases are testable such as simple select, update, delete, stored procedure, transaction

## Server Configuration
## Server Run
