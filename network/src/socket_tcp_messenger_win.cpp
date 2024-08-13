
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
    wsabuf.buf = _rCtx->_buf.data();
    wsabuf.len = (ULONG)_rCtx->_buf.size();
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
    else if (SUCCESS_RESULT == res && 0 < _rCtx->_bytes)
    {
        PostReceive();
        return InitReceive();
    }

    return true;
}

bool SocketTCPMessenger::PostSend()
{
    std::unique_lock<std::mutex> locker(_sendLock);

    std::string& buf = _sendBuf.front();
    if (buf.size() != _sCtx->_bytes)
    {
        ZS_LOG_ERROR(network, "sending buffer size is not equal with sent byte size, sock id : %llu, socket name : %s, peer : %s",
            _sockID, GetName(), GetPeer());
    }

    // reset the buffer just being completed for sending work
    // but the capacity of the buffer is reserved
    buf.clear();

    // move the buffer back to the back of the queue so that it would be reused
    if (Network::DEFAULT_TCP_SENDING_BUFFER_COUNT > _sendBufPool.size())
    {
        _sendBufPool.push_front(std::move(buf));
    }
    
    _sendBuf.pop();

    _sCtx->Reset();

    return !_sendBuf.empty();
}

bool SocketTCPMessenger::initSend()
{
    WSABUF wsabuf;
    std::string& buf = _sendBuf.front();
    wsabuf.buf = buf.data();
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
    else if (SUCCESS_RESULT == res && 0 < _sCtx->_bytes)
    {
        // processed directly
        if (true == PostSend())
        {
            return initSend();
        }
    }

    return true;
}

#endif // _WIN64_
