
#include    "network/connection.h"

#include    "common/log.h"

#include    "socket.h"

using namespace zs::common;
using namespace zs::network;

IPVer Connection::GetIPVer() const
{
    SocketSPtr sock = _sock.lock();
    if (nullptr != sock)
    {
        return sock->GetIPVer();
    }

    return IPVer::IP_INVALID;
}

Protocol Connection::GetProtocol() const
{
    SocketSPtr sock = _sock.lock();
    if (nullptr != sock)
    {
        return sock->GetProtocol();
    }

    return Protocol::INVALID;
}

void Connection::handleConnected()
{
    SocketSPtr sock = _sock.lock();
    if (nullptr != sock)
    {
        OnConnectedSPtr onConnected = sock->GetOnConnected();
        if (nullptr != onConnected)
        {
            (*onConnected)(shared_from_this());
        }
    }
}

void Connection::handleReceived()
{
    SocketSPtr sock = _sock.lock();
    if (nullptr != sock)
    {
        // get received data from socket
        // ** TCP **
        // append the data to received data buffer 
        // if the buffer is enough to make a message
        // then invoke onReceived with the message
        // ** UDP **
        // just invoke onReceived with the received data as a message
    }
}

Connection::Connection(ConnectionID id, SocketWPtr sock)
    : _id(id), _sock(sock)
{
    SocketSPtr s = _sock.lock();
    if (nullptr != s)
    {
        s->SetConnectionID(_id);
    }
}
