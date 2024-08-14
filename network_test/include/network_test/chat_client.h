#pragma once

#include    "common/types.h"
#include    "common/log.h"

#include    "network/network.h"
#include    "network/connection.h"

using namespace zs::common;
using namespace zs::network;

void ChatClient(Logger::Messenger msgr, IPVer ipVer, Protocol protocol, std::string serverHost, int32_t serverPort);

