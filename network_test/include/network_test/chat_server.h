#pragma once

#include    "common/types.h"
#include    "common/log.h"

#include    "network/network.h"
#include    "network/connection.h"

using namespace zs::common;
using namespace zs::network;

void ChatServer(Logger::Messenger msgr, IPVer ipVer, Protocol protocol, int32_t port);

