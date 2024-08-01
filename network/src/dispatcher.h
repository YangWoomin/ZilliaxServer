
#ifndef __ZS_NETWORK_DISPATCHER_H__
#define __ZS_NETWORK_DISPATCHER_H__

#include    "internal_common.h"

#if not defined(_MSVC_)
#include    <vector>
#include    <queue>
#endif // not _MSVC_

namespace zs
{
namespace network
{
    struct Result
    {
        bool        _isToBeStopped = false;
#if defined(_MSVC_)
        DWORD       _err = 0;
        DWORD       _bytes = 0;
#else // _MSVC_
        int32_t     _err = 0;
#endif // _MSVC_
    };

#if not defined(_MSVC_)
    struct Item final
    {
        Result              _res;
        SocketContext*      _sCtx = NULL;
        IOContext*          _iCtx = NULL;
    };

    class Epoll final
    {
    public:
        Epoll() = default;
        ~Epoll() = default;

        bool Initialize();
        void Close();
        void Finalize();

        void SetOwner(std::thread::id owner);
        bool Bind(SocketContext* ctx);
        bool Dequeue(std::queue<Item>& items);

    private:
        int32_t                 _epoll = -1;
        struct epoll_event*     _events = NULL;
        static const int32_t    MAX_EVENT_COUNT = 1024;

        int32_t                 _notiFd;

        std::thread::id         _owner;
    };
    using EpollSPtr = std::shared_ptr<Epoll>;

#endif // not _MSVC_

    class Dispatcher final
    {
    public:
        Dispatcher() = default;
        ~Dispatcher() = default;

        bool Initialize(std::size_t workerCount);
        void Close(std::size_t workerCount);
        void Finalize();

#if defined(_MSVC_)
    public:
        bool Bind(SocketContext* ctx);
        bool Dequeue(Result& res, SocketContext*& sCtx, IOContext*& iCtx);

    private:
        HANDLE                  _iocp = INVALID_HANDLE_VALUE;

#else // _MSVC_
    public:
        void SetOwner(std::size_t workerID, std::thread::id owner);
        bool Bind(std::size_t workerID, SocketContext* ctx);
        bool Dequeue(std::size_t workerID, std::queue<Item>& items);

    private:
        std::vector<EpollSPtr>  _epolls;

#endif // _MSVC_
    };
}
}

#endif // __ZS_NETWORK_DISPATCHER_H__
