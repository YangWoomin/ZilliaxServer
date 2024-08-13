
#include    "network/connection.h"

#include    "common/log.h"

#include    "socket.h"

using namespace zs::common;
using namespace zs::network;

// bool Connection::Send(const char* buf, std::size_t len)
// {
//     SocketSPtr sock = _sock.lock();
//     if (nullptr == sock)
//     {
//         ZS_LOG_ERROR(network, "invalid socket for send in connection");
//         return false;
//     }

//     if (false == sock->InitSend(buf, len))
//     {
//         ZS_LOG_ERROR(network, "init sending failed in connection");
//         return false;
//     }

//     return true;
// }

// bool Connection::Send(const std::string& buf)
// {
//     return this->Send(buf.c_str(), buf.size());
// }

bool Connection::Send(std::string&& buf)
{
    SocketSPtr sock = _sock.lock();
    if (nullptr == sock)
    {
        ZS_LOG_ERROR(network, "invalid socket for send in connection");
        return false;
    }

    if (false == sock->InitSend(std::move(buf)))
    {
        ZS_LOG_ERROR(network, "init sending failed in connection, sock id : %llu, socket name : %s",
            sock->GetID(), sock->GetName());
        return false;
    }

    return true;
}

void Connection::Close()
{
    SocketSPtr sock = _sock.lock();
    if (nullptr == sock)
    {
        ZS_LOG_ERROR(network, "invalid socket for send in connection");
        return;
    }

    sock->Close();
}

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

const char* Connection::GetPeer() const
{
    SocketSPtr sock = _sock.lock();
    if (nullptr != sock)
    {
        return sock->GetPeer();
    }

    return "unknown";
}

Connection::Connection(ConnectionID id, SocketWPtr sock)
    : _id(id), _sock(sock)
{
    
}
