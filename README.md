# ZilliaxServer

## Introduction
* Online MMO (RPG) Game Server
* 

## Features
* C/C++(20)
* Cross platform (Windows / Linux)
* Asynchronous network module (IOCP, epoll)
* ODBC database module
* 

## Dependencies
* ODBC Driver (MySQL)
* Make / CMake

## Development Envrionment

### IDE
* Visual Studio 2022
* Visual Studio Code

## Build

### Windows prerequisites
* os : Windows 10 (latest build)
* Visual Studio 2022 (Community)
* make : make-3.8.1.exe
* cmake : cmake-3.30.0-windows-x86_64.msi
* core utils : coreutils-5.3.0.exe
* mysql odbc driver : mysql-connector-odbc-9.0.0-win64.msi

### Linux (Ubuntu 22.04) prerequisites
* os : Ubuntu 24.04 LTS
* make : sudo apt install make
* cmake : sudo apt install cmake
* gcc/g++ : sudo apt install build-essential
* mysql odbc driver
    sudo apt install unixodbc-dev unixodbc
    sudo dpkg -i mysql-community-client-plugins_9.0.0-1ubuntu22.04_amd64.deb
    sudo apt update
    sudo apt install -f
    sudo dpkg -i mysql-connector-odbc_9.0.0-1ubuntu22.04_amd64.deb
    sudo apt update
    sudo apt install -f
    
### Build (Windows/Linux)
* configure : make configure
* reconfigure : make reconfigure
* build debug : make build_debug
* rebuild debug : make rebuild_debug
* build release : make build_release
* rebuild debug : make rebuild_release
* distclean : make distclean

