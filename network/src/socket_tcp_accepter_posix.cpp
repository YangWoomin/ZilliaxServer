
#include    "socket_tcp_accepter.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_POSIX_) 

bool SocketTCPAceepter::InitAccept()
{
    // no async accept in linux

    if (nullptr == _aCtx)
    {
        _aCtx = new AcceptContext();
    }
    _aCtx->Reset();

    return true;
}

bool SocketTCPAceepter::Accept()
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid listen socket, sock id : %llu, socket name : %s", 
            _sockID, GetName());
        return false;
    }

    if (nullptr == _aCtx)
    {
        ZS_LOG_ERROR(network, "invalid accept context, sock id : %llu, socket name : %s", 
            _sockID, GetName());
        return false;
    }

    _aCtx->_len = sizeof(sockaddr_storage);
    _aCtx->_sock = accept(_sock, (struct sockaddr*)_aCtx->_addr, &_aCtx->_len);
    if (INVALID_SOCKET == _aCtx->_sock)
    {
        ZS_LOG_ERROR(network, "accept failed, socket name : %s, err : %d", 
            GetName(), errno);
        return false;
    }

    return true;
}

bool SocketTCPAceepter::postAccept(std::string& name, std::string& peer)
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

    if (false == Helper::MakeSocketNonBlocking(_aCtx->_sock))
    {
        ZS_LOG_ERROR(network, "making accepted socket non-blocking failed, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    int32_t localPort = 0;
    Helper::GetSockLocalAddr(_aCtx->_sock, _ipVer, name, localPort);
    name = name.c_str();
    name += (":" + std::to_string(localPort));

    int32_t remotePort = 0;
    Helper::GetSockAddr((sockaddr*)_aCtx->_addr, peer, remotePort);
    peer = peer.c_str();
    peer += (":" + std::to_string(remotePort));
    
    return true;
}

#endif // defined(_POSIX_) 
