#pragma once

#include    "common/types.h"
#include    "common/log.h"

#include    "network/network.h"
#include    "network/connection.h"

#include    <vector>
#include    <atomic>

using namespace zs::common;
using namespace zs::network;

void ChatMassiveTestClient(Logger::Messenger msgr, IPVer ipVer, Protocol protocol, std::string serverHost, int32_t serverPort, std::size_t clientCount, std::string sampleFileDir);

class ChatMassiveTester
{
public:
    bool Connect(IPVer ipVer, Protocol protocol, const std::string& serverHost, int32_t serverPort);
    void Send(const std::vector<std::string>& text);
    std::size_t GetConnID() const { return _server->GetID(); }
    
    ChatMassiveTester() = default;
    ~ChatMassiveTester() = default;

private:
    ConnectionSPtr              _server;
    std::atomic<std::size_t>    _recvMsgCount { 0 };
};

