# ZilliaxServer

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
* Asynchronous network module (IOCP, epoll) ===> in progress
* Seamless open world framework (geographical separating shards synchronized by inter-server, which means technologically distributed system) ===> after Asynchronous network module

## Build Tool / Driver
* Make/CMake
* MSVC/g++
* ODBC Driver (MySQL)

## License
* ZilliaxServer : Apache 2.0
* spdlog-1.14.1 ([https://github.com/gabime/spdlog](https://github.com/gabime/spdlog/tree/v1.14.1)) - MIT License

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

### Linux (Ubuntu 24.04 on wsl2) Prerequisites
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
    
### Build 
#### "x64 Native Tools Command Prompt for VS 2022" on Windows or bash shell on Linux
* configure : make configure
* reconfigure : make reconfigure
* build debug : make build_debug
* rebuild debug : make rebuild_debug
* build release : make build_release
* rebuild debug : make rebuild_release
* distclean : make distclean
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
* click the play icon
### VS Code on Linux (wsl2)
* install gdb on    
  + sudo apt install gdb
* VS Code setting for WSL : https://code.visualstudio.com/docs/remote/wsl
* install "WSL" extension of Visual Studio Code
* open VS Code from WSL cmd or connect to WSL from VS Code on Windows
  + ![image](https://github.com/user-attachments/assets/1aedf17e-991a-4675-bfae-eaa0e27e6872)
* Ctrl + Shift + D
* select one of the launch list you want to debug
* click the play icon

## Database
* env : Ubuntu 24.04 on wsl2 + docker + docker-compose + MySQL Workbench

### Docker & Docker Compose Installation 
* docker-ce : https://docs.docker.com/engine/install/ubuntu/
* docker-compose : https://docs.docker.com/compose/install/standalone/#on-linux
* sudo usermod -aG docker $USER 
* newgrp docker

### MySQL on Docker
* move cmd(bash shell) current working directory to ./setting/database/mysql
* docker-compose up -d
* docker-compose ps

### "db" Module Test
#### initial scripts
* run ./setting/database/mysql/scripts/db_test/init.sql by MySQL Workbench 
#### "db" Module Build
* move cmd current working directory to ./db
* build db module by "make distclean && make rebuild_debug"
#### "db_test" Tester Build
* move cmd current working directory to ./db_test
* build db_test by "make rebuild_debug"
#### Test
* some cases are testable such as simple select, update, delete, stored procedure, transaction

## Server Configuration
## Server Run
