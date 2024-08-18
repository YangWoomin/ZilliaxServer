
#ifndef __ZS_NETWORK_SOCKET_TCP_MESSENGER_H__
#define __ZS_NETWORK_SOCKET_TCP_MESSENGER_H__

#include    "network/network.h"
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
        virtual bool InitSend(const std::string& buf) override;
        virtual bool InitSend(const char* buf, std::size_t len) override;

        virtual bool ContinueSend() override;

        virtual bool InitReceive() override;

#if defined(_POSIX_) 
        virtual bool Receive(bool& later) override;
#endif // defined(_POSIX_) 

        virtual bool PostSend() override;

        virtual bool PostReceive() override;

    private:
        bool initSend();
        bool send();

        std::queue<std::vector<uint8_t>>    _sendBuf;
        std::deque<std::vector<uint8_t>>    _sendBufPool;
        Lock                                _sendLock;

        std::vector<uint8_t>                _recvBuf;
        uint32_t                            _curMsgLen = 0;
        Lock                                _recvLock;

        SocketTCPMessenger(Manager& manager, SocketID sockID, IPVer ipVer, bool nonBlocking);
        SocketTCPMessenger(Manager& manager, SocketID sockID, Socket sock, const std::string& name, const std::string& peer, IPVer ipVer);

        SocketTCPMessenger(const SocketTCPMessenger&) = delete;
        SocketTCPMessenger(const SocketTCPMessenger&&) = delete;
        SocketTCPMessenger& operator=(const SocketTCPMessenger&) = delete;

    public:
        virtual ~SocketTCPMessenger();

        friend class SocketGenerator;
        friend class SocketTCPConnector;

    private:
        template <typename T>
        bool initSend(T& buf, std::size_t len)
        {
            if (INVALID_SOCKET == _sock)
            {
                ZS_LOG_ERROR(network, "invalid messenger socket in init send, sock id : %llu, socket name : %s, peer : %s", 
                    _sockID, GetName(), GetPeer());
                return false;
            }

            uint32_t len2 = static_cast<uint32_t>(len);
            len2 = htonl(len2);

            {
                AutoScopeLock locker(&_sendLock);

                bool isNewBufferNeeded = false;

                if (true == _sendBuf.empty())
                {
                    // no data being sent yet
                    isNewBufferNeeded = true;
                }
                else if (1 == _sendBuf.size())
                {
                    // already first element is being sent
                    isNewBufferNeeded = true;
                }
                else
                {
                    std::vector<uint8_t>& old = _sendBuf.back();
                    if (DEFAULT_TCP_SENDING_BUFFER_SIZE < old.size() + sizeof(len2) + len)
                    {
                        isNewBufferNeeded = true;
                    }
                    else
                    {
                        // append to the last old buffer
                        old.insert(old.end(), (uint8_t*)&len2, (uint8_t*)&len2 + sizeof(len2));
                        old.insert(old.end(), (uint8_t*)&buf[0], (uint8_t*)&buf[0] + len);
                    }
                }

                if (true == isNewBufferNeeded)
                {
                    std::vector<uint8_t> newBuf;
                    if (0 < _sendBufPool.size())
                    {
                        // reuse buffer
                        newBuf = std::move(_sendBufPool.front());
                        _sendBufPool.pop_front();
                    }
                    else
                    {
                        newBuf.reserve(DEFAULT_TCP_SENDING_BUFFER_SIZE);
                    }
                    newBuf.insert(newBuf.end(), (uint8_t*)&len2, (uint8_t*)&len2 + sizeof(len2));
                    newBuf.insert(newBuf.end(), (uint8_t*)&buf[0], (uint8_t*)&buf[0] + len);
                    _sendBuf.push(std::move(newBuf));
                }

                if (1 < _sendBuf.size())
                {
                    // just hand over sending work to the currently working thread
                    return true;
                }

                if (nullptr == _sCtx)
                {
                    _sCtx = new SendRecvContext();
                }
                _sCtx->Reset();
            }
            
            if (false == initSend())
            {
                ZS_LOG_WARN(network, "internal init send failed, sock id : %llu, socket name : %s, peer : %s", 
                    _sockID, GetName(), GetPeer());
                Close();
                return false;
            }

            return true;
        }
    };
}
}

#endif // __ZS_NETWORK_SOCKET_TCP_MESSENGER_H__
