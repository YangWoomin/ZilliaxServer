
#include    "socket_tcp_connector.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_POSIX_) 

bool SocketTCPConnector::initConnect(std::size_t idx)
{
    if (INVALID_RESULT == connect(_sock, (const sockaddr*)_cCtx->_addrs[idx]._addr, _cCtx->_addrs[idx]._len))
    {
        int err = errno;
        if (EINPROGRESS != err)
        {
            ZS_LOG_ERROR(network, "connect failed, sock id : %llu, socket name : %s, err : %d",
                _sockID, GetName(), err);
            return false;
        }
    }

    ZS_LOG_INFO(network, "init connecting succeeded, socket id : %llu, socket name : %s", 
        _sockID, GetName());

    return true;
}

bool SocketTCPConnector::postConnect()
{
    // unbind the connected socket from dispatcher for connected event
    // bind the connected socket to dispatcher for data received event
    if (false == _manager.Bind(_workerID, this, BindType::MODIFY, EventType::INBOUND))
    {
        ZS_LOG_ERROR(network, "binding connector socket on dispatcher failed, sock id : %llu, socket name : %s, peer : %s",
            _sockID, GetName(), GetPeer());
        return false;
    }

    return true;
}

#endif // defined(_POSIX_) 