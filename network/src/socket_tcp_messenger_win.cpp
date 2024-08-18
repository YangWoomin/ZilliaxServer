
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

    return true;
}

bool SocketTCPMessenger::PostSend()
{
    AutoScopeLock locker(&_sendLock);

    if (true == _sendBuf.empty())
    {
        return false;
    }

    if (0 == _sCtx->_bytes)
    {
        ZS_LOG_WARN(network, "sent byte size is invalid(0), sock id : %llu, socket name : %s, peer : %s",
            _sockID, GetName(), GetPeer());
        return true; // retry to send
    }

    std::vector<uint8_t>& buf = _sendBuf.front();
    if (buf.size() <= _sCtx->_bytes)
    {
        // all data is sent in the buffer
        buf.clear();

        if (DEFAULT_TCP_SENDING_BUFFER_COUNT > _sendBufPool.size())
        {
            _sendBufPool.push_front(std::move(buf));
        }

        _sendBuf.pop();

        _sCtx->Reset();
    }

    return !_sendBuf.empty();
}

bool SocketTCPMessenger::initSend()
{
    return this->send();
}

bool SocketTCPMessenger::send()
{
    WSABUF wsabuf;
    DWORD sentBytes = 0;
    std::vector<uint8_t>& buf = _sendBuf.front();
    wsabuf.buf = (char*)buf.data() + _sCtx->_bytes;
    wsabuf.len = (ULONG)buf.size() - _sCtx->_bytes;

    int res = WSASend(_sock, &wsabuf, 1, &sentBytes, 0, &_sCtx->_ol, NULL);
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

    return true;
}

#endif // _WIN64_
