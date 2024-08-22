
#ifndef __ZS_NETWORK_DISPATCHER_H__
#define __ZS_NETWORK_DISPATCHER_H__

#include    "internal_common.h"

namespace zs
{
namespace network
{
    class ISocket;
    
#if defined(_POSIX_) 
    class Epoll final
    {
    public:
        Epoll() = default;
        ~Epoll() = default;

        bool Initialize();
        void Close();
        void Finalize();

        void SetOwner(std::size_t workerID, std::thread::id tid);
        bool Bind(ISocket* sock, BindType bindType, EventType eventType);
        bool Dequeue(std::queue<IOResult>& resList);
        
        void Release(SocketSPtr sock);

    private:
        int32_t                 _epoll = INVALID_FD_VALUE;
        struct epoll_event*     _events = NULL;
        static const int32_t    MAX_EVENT_COUNT = 1024;
        static const uint64_t   CLOSE_SIGNAL = 1;
        static const uint64_t   RELEASE_SIGNAL = 2;

        int32_t                 _closeNotiFd;
        int32_t                 _releaseNotiFd;

        std::mutex              _lock;
        std::queue<SocketSPtr>  _releaseSocks;

        std::size_t             _workerID;
        std::thread::id         _tid;

        bool bindEvent(int32_t& fd);
    };
    using EpollSPtr = std::shared_ptr<Epoll>;

#endif // defined(_POSIX_) 

    class Dispatcher final
    {
    public:
        Dispatcher() = default;
        ~Dispatcher() = default;

        bool Initialize(std::size_t workerCount);
        void Stop(std::size_t workerCount);
        void Finalize();
        
        bool Dequeue(std::size_t workerID, std::queue<IOResult>& resList);

#if defined(_WIN64_)
    public:
        bool Bind(ISocket* sock);
        bool Release(SocketSPtr sock);

    private:
        HANDLE                  _iocp = INVALID_HANDLE_VALUE;

#elif defined(_POSIX_) 
    public:
        void SetOwner(std::size_t workerID, std::thread::id tid);
        bool Bind(std::size_t workerID, ISocket* sock, BindType bindType, EventType eventType);
        void Release(std::size_t workerID, SocketSPtr sock);

    private:
        std::vector<EpollSPtr>  _epolls;

#endif // _WIN64_
    };
}
}

#endif // __ZS_NETWORK_DISPATCHER_H__
