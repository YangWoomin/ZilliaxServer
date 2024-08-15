
#include    "socket_tcp_messenger.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"
#include    "network/connection.h"

using namespace zs::common;
using namespace zs::network;

bool SocketTCPMessenger::InitSend(std::string&& buf)
{
    return InitSend(buf);
}

bool SocketTCPMessenger::InitSend(std::string& buf)
{
    if (0 >= buf.size())
    {
        ZS_LOG_ERROR(network, "invalid bufffer in init send, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }

    if (DEFAULT_TCP_SENDING_BUFFER_SIZE < buf.size())
    {
        ZS_LOG_ERROR(network, "sending data is too big, check DEFAULT_TCP_SENDING_BUFFER_SIZE in common.h, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }

    return initSend(buf, buf.size());
}

bool SocketTCPMessenger::InitSend(const char* buf, std::size_t len)
{
    if (nullptr == buf || 0 == len)
    {
        ZS_LOG_ERROR(network, "invalid bufffer in init send, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }

    if (DEFAULT_TCP_SENDING_BUFFER_SIZE < len)
    {
        ZS_LOG_ERROR(network, "sending data is too big, check DEFAULT_TCP_SENDING_BUFFER_SIZE in common.h, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }

    return initSend(buf, len);
}

bool SocketTCPMessenger::ContinueSend()
{
    return this->send();
}

bool SocketTCPMessenger::PostReceive()
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid messenger socket in post receive, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }

    if (nullptr == _conn)
    {
        ZS_LOG_ERROR(network, "invalid connection in post receive, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }

    if (nullptr == _rCtx)
    {
        ZS_LOG_ERROR(network, "invalid recv context in post receive, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }

    if (0 == _rCtx->_bytes)
    {
        // socket closed by receiving 0 byte
        ZS_LOG_WARN(network, "connection is being closed, sock id : %llu, socket name : %s, peer : %s",
            _sockID, GetName(), GetPeer());
        return false;
    }

    // ** TCP **
    // get received data from socket
    // append the data to received data buffer 
    // if the buffer is enough to make a message
    // then invoke onReceived with the message

    // no need for lock when receiving data
    //std::unique_lock<std::mutex> locker(_recvLock);

    _recvBuf.insert(_recvBuf.end(), _rCtx->_buf, _rCtx->_buf + _rCtx->_bytes);

    while (true)
    {
        if (0 == _curMsgLen && sizeof(_curMsgLen) < _recvBuf.size())
        {
            _curMsgLen = *(reinterpret_cast<uint32_t*>(_recvBuf.data()));
            _curMsgLen = ntohl(_curMsgLen);
            _recvBuf.erase(_recvBuf.begin(), _recvBuf.begin() + sizeof(_curMsgLen));
        }

        if (0 < _curMsgLen && _recvBuf.size() >= _curMsgLen)
        {
            if (nullptr != _onReceived)
            {
                //(*_onReceived)(_conn, (const char*)_recvBuf.data(), _curMsgLen);
                std::string data((const char*)_recvBuf.data(), _curMsgLen);
                _onReceived(_conn, data.c_str(), data.size());
            }

            _recvBuf.erase(_recvBuf.begin(), _recvBuf.begin() + _curMsgLen);
            _curMsgLen = 0;
            continue;
        }

        break;
    }
    
    // ** UDP **
    // just invoke onReceived with the received data as a message

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
