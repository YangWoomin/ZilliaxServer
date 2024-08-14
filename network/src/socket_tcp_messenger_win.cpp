
#include    "socket_tcp_messenger.h"

#include    "common/log.h"

#include    "network/network.h"
#include    "helper.h"
#include    "manager.h"
#include    "network/connection.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_WIN64_)

bool SocketTCPMessenger::InitReceive()
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid socket in init receive, sock id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }

    if (nullptr == _rCtx)
    {
        _rCtx = new SendRecvContext();
    }
    _rCtx->Reset();

    WSABUF wsabuf;
    wsabuf.buf = _rCtx->_buf;
    wsabuf.len = (ULONG)sizeof(_rCtx->_buf);
    DWORD flags = 0;
    int res = WSARecv(_sock, &wsabuf, 1, &_rCtx->_bytes, &flags, &_rCtx->_ol, NULL);
    if (SOCKET_ERROR == res)
    {
        int err = errno;
        if (WSA_IO_PENDING != err)
        {
            ZS_LOG_ERROR(network, "WSARecv failed, sock id : %llu, socket name : %s, peer : %s", 
                _sockID, GetName(), GetPeer());
            return false;
        }
    }
    else if (NO_ERROR == res && 0 < _rCtx->_bytes)
    {
        PostReceive();
        return InitReceive();
    }

    return true;
}

bool SocketTCPMessenger::PostSend()
{
    std::unique_lock<std::mutex> locker(_sendLock);

    if (true == _sendBuf.empty())
    {
        return false;
    }

    std::vector<uint8_t>& buf = _sendBuf.front();
    if (buf.size() != _sCtx->_bytes)
    {
        ZS_LOG_ERROR(network, "sending buffer size is not equal with sent byte size, sock id : %llu, socket name : %s, peer : %s",
            _sockID, GetName(), GetPeer());
    }

    // reset the buffer just being completed for sending work
    // but the capacity of the buffer is reserved
    buf.clear();

    // move the buffer back to the back of the queue so that it would be reused
    if (DEFAULT_TCP_SENDING_BUFFER_COUNT > _sendBufPool.size())
    {
        _sendBufPool.push_front(std::move(buf));
    }
    
    _sendBuf.pop();

    _sCtx->Reset();

    return !_sendBuf.empty();
}

bool SocketTCPMessenger::initSend()
{
    return this->send();
}

bool SocketTCPMessenger::send()
{
    WSABUF wsabuf;
    std::vector<uint8_t>& buf = _sendBuf.front();
    std::memcpy(_sCtx->_buf, buf.data(), buf.size());
    wsabuf.buf = _sCtx->_buf;
    wsabuf.len = (ULONG)buf.size();

    int res = WSASend(_sock, &wsabuf, 1, &_sCtx->_bytes, 0, &_sCtx->_ol, NULL);
    if (SOCKET_ERROR == res)
    {
        int err = errno;
        if (WSA_IO_PENDING != err)
        {
            ZS_LOG_ERROR(network, "WSASend failed, sock id : %llu, socket name : %s, peer : %s", 
                _sockID, GetName(), GetPeer());
            return false;
        }
    }
    else if (NO_ERROR == res && 0 < _sCtx->_bytes)
    {
        // processed directly
        if (true == PostSend())
        {
            return this->send();
        }
    }

    return true;
}

#endif // _WIN64_
