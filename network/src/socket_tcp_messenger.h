
#ifndef __ZS_NETWORK_SOCKET_TCP_MESSENGER_H__
#define __ZS_NETWORK_SOCKET_TCP_MESSENGER_H__

#include    "socket.h"

namespace zs
{
namespace network
{
    class Manager;

    class SocketTCPMessenger : public ISocket
    {
    public:
        virtual bool InitSend(std::string&& buf) override;

        virtual bool ContinueSend() override;

        virtual bool InitReceive() override;

#if defined(_POSIX_) 
        virtual bool OnReceived(bool& later) override;
#endif // defined(_POSIX_) 

        virtual bool PostSend() override;

        virtual bool PostReceive() override;

    private:
        bool initSend();
        bool send();

        std::queue<std::string>     _sendBuf;
        std::deque<std::string>     _sendBufPool;
        std::mutex                  _sendLock;

        std::string                 _recvBuf;
        uint32_t                    _curMsgLen = 0;
        std::mutex                  _recvLock;

        SocketTCPMessenger(Manager& manager, SocketID sockID, IPVer ipVer, bool nonBlocking);
        SocketTCPMessenger(Manager& manager, SocketID sockID, Socket sock, const std::string& name, const std::string& peer, IPVer ipVer);

        SocketTCPMessenger(const SocketTCPMessenger&) = delete;
        SocketTCPMessenger(const SocketTCPMessenger&&) = delete;
        SocketTCPMessenger& operator=(const SocketTCPMessenger&) = delete;

    public:
        virtual ~SocketTCPMessenger();

        friend class SocketGenerator;
        friend class SocketTCPConnector;
    };
}
}

#endif // __ZS_NETWORK_SOCKET_TCP_MESSENGER_H__
