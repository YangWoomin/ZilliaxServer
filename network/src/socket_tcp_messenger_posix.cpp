
#include    "socket_tcp_messenger.h"

#include    "common/log.h"

#include    "network/network.h"
#include    "helper.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_LINUX_) 

bool SocketTCPMessenger::InitReceive()
{
    // no init receive on linux
    
    return true;
}

bool SocketTCPMessenger::OnReceived(bool& later)
{


    return true;
}

bool SocketTCPMessenger::PostSend()
{
    std::unique_lock<std::mutex> locker(_sendLock);

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

    return !_sendBuf.empty();
}

bool SocketTCPMessenger::initSend()
{
    std::string& buf = _sendBuf.front();

    ssize_t bytes = send(_sock, buf.data() + _sCtx->_bytes, buf.size() - _sCtx->_bytes, 0);
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
            return initSend();
        }
    }

    return true;
}

#endif // defined(_LINUX_) 
