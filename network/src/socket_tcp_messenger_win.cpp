
#include    "socket_tcp_messenger.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_MSVC_)

bool SocketTCPMessenger::PreRecv(bool& isReceived)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid messenger socket, sock id : %llu, socket name : %s, peer : %s", 
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
    wsabuf.len = BUFFER_SIZE;
    DWORD flags = 0;
    DWORD bytes = 0;
    int res = WSARecv(_sock, &wsabuf, 1, &bytes, &flags, &_rCtx->_ol, NULL);
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
    else if (SUCCESS_RESULT == res && 0 < bytes)
    {
        isReceived = true;
    }

    return true;
}

#endif // _MSVC_
