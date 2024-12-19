#pragma once

#include    "common/types.h"
#include    "common/log.h"

#include    "network/network.h"
#include    "network/connection.h"

#include    <functional>

using namespace zs::common;
using namespace zs::network;

using OnClientConnected = std::function<void(const char*)>;
using OnClientClosed = std::function<void(const char*)>;
using OnMessageReceived = std::function<void(const char*, const char*, std::size_t)>;

void ChatServer(Logger::Messenger msgr, std::size_t workerCount, IPVer ipVer, Protocol protocol, int32_t port, bool isBroadcasting, OnClientConnected onClientConnected, OnClientClosed onClientClosed, OnMessageReceived onMessageReceived);

