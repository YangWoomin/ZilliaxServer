
#include    "socket_tcp_messenger.h"

#include    "common/log.h"

#include    "network/network.h"
#include    "helper.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_POSIX_) 

bool SocketTCPMessenger::InitReceive()
{
    if (nullptr == _rCtx)
    {
        _rCtx = new SendRecvContext();
        _rCtx->_buf.resize(Network::DEFAULT_TCP_RECVING_BUFFER_SIZE);
    }
    _rCtx->Reset();
    
    return true;
}

bool SocketTCPMessenger::OnReceived(bool& later)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid socket in on received, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }

    if (nullptr == _rCtx)
    {
        ZS_LOG_ERROR(network, "invalid recv context in on received, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }

    _rCtx->_bytes = recv(_sock, _rCtx->_buf.data(), _rCtx->_buf.size(), 0);
    int err = errno;
    if (SOCKET_ERROR == _rCtx->_bytes)
    {
        if (EAGAIN == err || EWOULDBLOCK == err)
        {
            ZS_LOG_WARN(network, "no data for recv, sock id : %llu, socket name : %s, peer : %s",
                _sockID, GetName(), GetPeer());
            later = true;
            return true;
        }

        ZS_LOG_ERROR(network, "recv failed in on received, sock id : %llu, socket name : %s, peer : %s, err : %d",
            _sockID, GetName(), GetPeer(), err);
        return false;
    }

    return true;
}

bool SocketTCPMessenger::PostSend()
{
    std::unique_lock<std::mutex> locker(_sendLock);

    if (true == _sendBuf.empty())
    {
        // the first time the socket is registered on epoll
        // it receives signal that the socket sending buffer is available
        return false;
    }

    std::string& buf = _sendBuf.front();
    if ((int)buf.size() <= _sCtx->_bytes)
    {
        // all data is sent in the buffer
        buf.clear();

        if (Network::DEFAULT_TCP_SENDING_BUFFER_COUNT > _sendBufPool.size())
        {
            _sendBufPool.push_front(std::move(buf));
        }

        _sendBuf.pop();

        _sCtx->Reset();
    }

    if (true == _sendBuf.empty())
    {
        // unbind out-bound event on dispatcher
        if (false == _manager.Bind(_workerID, this, BindType::MODIFY, EventType::INBOUND))
        {
            ZS_LOG_ERROR(network, "binding failed in post send, sock id : %llu, socket name : %s, peer : %s", 
                _sockID, GetName(), GetPeer());
        }
    }

    return !_sendBuf.empty();
}

bool SocketTCPMessenger::initSend()
{
    // bind on dispatcher to trigger sending buffer available event
    if (false == _manager.Bind(_workerID, this, BindType::MODIFY, (EventType)(EventType::INBOUND | EventType::OUTBOUND)))
    {
        ZS_LOG_ERROR(network, "binding failed in init send, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }

    return this->send();
}

bool SocketTCPMessenger::send()
{
    std::string& buf = _sendBuf.front();

    ssize_t bytes = ::send(_sock, buf.data() + _sCtx->_bytes, buf.size() - _sCtx->_bytes, MSG_NOSIGNAL);
    if (SOCKET_ERROR == bytes)
    {
        int err = errno;
        if (EAGAIN != err && EWOULDBLOCK != err)
        {
            ZS_LOG_ERROR(network, "send failed in init send, sock id : %llu, socket name : %s, peer : %s", 
                _sockID, GetName(), GetPeer());
            return false;
        }
    }
    else if (0 < bytes)
    {
        _sCtx->_bytes += bytes;
        if (true == PostSend())
        {
            return this->send();
        }
    }

    return true;
}

#endif // defined(_POSIX_) 