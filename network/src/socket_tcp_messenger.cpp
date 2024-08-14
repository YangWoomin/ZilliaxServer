
#include    "socket_tcp_messenger.h"

#include    "common/log.h"

#include    "network/network.h"
#include    "helper.h"
#include    "manager.h"
#include    "network/connection.h"

using namespace zs::common;
using namespace zs::network;

bool SocketTCPMessenger::InitSend(std::string&& buf)
{
    if (0 >= buf.size())
    {
        ZS_LOG_ERROR(network, "invalid bufffer in init send, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }

    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid messenger socket in init send, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }

    uint32_t msgLen = static_cast<uint32_t>(buf.size());
    msgLen = htonl(msgLen);
    std::string msgLenStr((const char*)&msgLen, sizeof(u_long));

    {
        std::unique_lock<std::mutex> locker(_sendLock);

        bool isNewBufferNeeded = false;

        if (true == _sendBuf.empty())
        {
            // no data being sent yet
            isNewBufferNeeded = true;
        }
        else if (1 == _sendBuf.size())
        {
            // already first element is being sent
            isNewBufferNeeded = true;
        }
        else
        {
            std::string& old = _sendBuf.back();
            if (Network::DEFAULT_TCP_SENDING_BUFFER_SIZE < old.size() + msgLenStr.size() + buf.size())
            {
                isNewBufferNeeded = true;
            }
            else
            {
                // append to the last old buffer
                old.append(msgLenStr);
                old.append(buf);
            }
        }

        if (true == isNewBufferNeeded)
        {
            std::string newBuf;
            if (0 < _sendBufPool.size())
            {
                // reuse buffer
                newBuf = std::move(_sendBufPool.front());
                _sendBufPool.pop_front();
            }
            else
            {
                newBuf.reserve(Network::DEFAULT_TCP_SENDING_BUFFER_SIZE);
            }
            newBuf.append(msgLenStr);
            newBuf.append(buf);
            _sendBuf.push(std::move(newBuf));
        }

        if (1 < _sendBuf.size())
        {
            // just hand over sending work to the currently working thread
            return true;
        }
    }
    
    // this thread sends the remote the buffer directly and asynchronously
    if (nullptr == _sCtx)
    {
        _sCtx = new SendRecvContext();
    }
    _sCtx->Reset();

    if (false == initSend())
    {
        ZS_LOG_WARN(network, "internal init send failed, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        Close();
        return false;
    }

    return true;
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

    // get received data from socket
    // ** TCP **
    // append the data to received data buffer 
    // if the buffer is enough to make a message
    // then invoke onReceived with the message

    std::unique_lock<std::mutex> locker(_recvLock);

    _recvBuf.append(_rCtx->_buf);

    if (0 == _curMsgLen && sizeof(_curMsgLen) < _recvBuf.size())
    {
        _curMsgLen = *(reinterpret_cast<uint32_t*>(_recvBuf.data()));
        _curMsgLen = ntohl(_curMsgLen);
        _recvBuf.erase(0, sizeof(_curMsgLen));
    }

    if (0 < _curMsgLen && _recvBuf.size() >= _curMsgLen)
    {
        if (nullptr != _onReceived)
        {
            (*_onReceived)(_conn, _recvBuf.data(), _curMsgLen);
        }
        _recvBuf.erase(0, _curMsgLen);
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
