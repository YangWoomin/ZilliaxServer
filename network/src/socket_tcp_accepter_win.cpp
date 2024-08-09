
#include    "socket_tcp_accepter.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_WIN64_)

bool SocketTCPAceepter::PreAccept()
{
    // AcceptEx
    if (nullptr == Helper::_lpfnAcceptEx)
    {
        ZS_LOG_ERROR(network, "invalid acceptex, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    Socket sock = Helper::CreateSocket(_ipVer, _protocol);
    if (INVALID_SOCKET == sock)
    {
        ZS_LOG_ERROR(network, "creating socket for pre accept failed, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    if (nullptr == _aCtx)
    {
        _aCtx = new AcceptContext();
    }
    _aCtx->Reset();
    _aCtx->_sock = sock;
    if (FALSE == Helper::_lpfnAcceptEx(_sock, sock, _aCtx->_addr, 0, sizeof(sockaddr_storage) + 16, sizeof(sockaddr_storage) + 16, &_aCtx->_len, &_aCtx->_ol))
    {
        int err = errno;
        if (WSA_IO_PENDING != err)
        {
            ZS_LOG_ERROR(network, "acceptex failed, sock id : %llu, socket name : %s, err : %d",
                _sockID, GetName(), err);
            CloseSocket(sock);
            return false;
        }
    }

    ZS_LOG_INFO(network, "pre accepting succeeded, socket id : %llu, socket name : %s", 
        _sockID, GetName());

    return true;
}

bool SocketTCPAceepter::PostAccept(std::string& name, std::string& peer)
{
    if (nullptr == _aCtx)
    {
        ZS_LOG_ERROR(network, "invalid accept context, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    if (INVALID_SOCKET == _aCtx->_sock)
    {
        ZS_LOG_ERROR(network, "invalid accepted socket, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    if (nullptr == Helper::_lpfnGetAcceptExSockAddr)
    {
        ZS_LOG_ERROR(network, "invalid getacceptexsockaddr, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    if (SOCKET_ERROR == setsockopt(_aCtx->_sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const char*)&_sock, sizeof(_sock)))
    {
        ZS_LOG_ERROR(network, "updating accepted socket failed, sock id : %llu, socket name : %s, err : %d",
            _sockID, GetName(), errno);
        return false;
    }

    sockaddr* localAddr = nullptr;
    int localLen = sizeof(sockaddr_storage);
    sockaddr* remoteAddr = nullptr;
    int remoteLen = sizeof(sockaddr_storage);
    Helper::_lpfnGetAcceptExSockAddr(_aCtx->_addr, 0, sizeof(sockaddr_storage) + 16, sizeof(sockaddr_storage) + 16, &localAddr, &localLen, &remoteAddr, &remoteLen);
    if (nullptr == localAddr || nullptr == remoteAddr)
    {
        ZS_LOG_ERROR(network, "GetAcceptExSockaddrs failed in post accept, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    int32_t localPort = 0;
    int32_t remotePort = 0;
    Helper::GetSockAddr(localAddr, name, localPort);
    Helper::GetSockAddr(remoteAddr, peer, remotePort);
    if (true == name.empty() || true == peer.empty())
    {
        ZS_LOG_ERROR(network, "GetSockAddr failed in post accept, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    name += (":" + std::to_string(localPort));
    peer += (":" + std::to_string(remotePort));
    
    return true;
}

#endif // _WIN64_

