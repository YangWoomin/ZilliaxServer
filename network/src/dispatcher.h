
#ifndef __ZS_NETWORK_DISPATCHER_H__
#define __ZS_NETWORK_DISPATCHER_H__

#include    "internal_common.h"

#if defined(__GNUC__) || defined(__clang__)
#include    <vector>
#endif // defined(__GNUC__) || defined(__clang__)
#include    <queue>

namespace zs
{
namespace network
{
    class ISocket;
    
#if defined(__GNUC__) || defined(__clang__)
    class Epoll final
    {
    public:
        Epoll() = default;
        ~Epoll() = default;

        bool Initialize();
        void Close();
        void Finalize();

        void SetOwner(std::size_t workerID);
        bool Bind(ISocket* sock, BindType bindType, EventType eventType);
        bool Dequeue(std::queue<ResultItem>& items);

    private:
        int32_t                 _epoll = INVALID_FD_VALUE;
        struct epoll_event*     _events = NULL;
        static const int32_t    MAX_EVENT_COUNT = 1024;
        static const uint64_t   CLOSE_SIGNAL = 1;

        int32_t                 _notiFd;

        std::size_t             _workerID;
    };
    using EpollSPtr = std::shared_ptr<Epoll>;

#endif // defined(__GNUC__) || defined(__clang__)

    class Dispatcher final
    {
    public:
        Dispatcher() = default;
        ~Dispatcher() = default;

        bool Initialize(std::size_t workerCount);
        void Stop(std::size_t workerCount);
        void Finalize();
        
        bool Dequeue(std::size_t workerID, std::queue<ResultItem>& items);

#if defined(_MSVC_)
    public:
        bool Bind(ISocket* sock);

    private:
        HANDLE                  _iocp = INVALID_HANDLE_VALUE;

#elif defined(__GNUC__) || defined(__clang__)
    public:
        void SetOwner(std::size_t workerID);
        bool Bind(std::size_t workerID, ISocket* sock, BindType bindType, EventType eventType);

    private:
        std::vector<EpollSPtr>  _epolls;

#endif // _MSVC_
    };
}
}

#endif // __ZS_NETWORK_DISPATCHER_H__
