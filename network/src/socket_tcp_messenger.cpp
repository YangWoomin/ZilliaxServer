
#include    "socket_tcp_messenger.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

bool SocketTCPMessenger::Send()
{


    return true;
}

SocketTCPMessenger::~SocketTCPMessenger()
{
    Close();
}

SocketTCPMessenger::SocketTCPMessenger(Manager& manager, SocketID sockID, IPVer ipVer, bool nonBlocking)
    : ISocket(manager, sockID, SocketType::MESSENGER, ipVer, Protocol::TCP, nonBlocking)
{

}

SocketTCPMessenger::SocketTCPMessenger(Manager& manager, SocketID sockID, Socket sock, const std::string& name, const std::string& peer, IPVer ipVer)
    : ISocket(manager, sockID, sock, name, peer, SocketType::MESSENGER, ipVer, Protocol::TCP)
{

}
