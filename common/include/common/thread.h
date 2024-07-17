
#ifndef __ZS_COMMON_THREAD_H__
#define __ZS_COMMON_THREAD_H__

#include    <atomic>
#include    <thread>

namespace zs
{
namespace common
{
    enum ThreadStatus
    {
        THREAD_STATUS_STANDBY = 0,
        THREAD_STATUS_RUNNING = 1,
        THREAD_STATUS_TERMINATED = 1,
    };

    template <typename Derived>
    class Thread
    {
    public:
        bool Start(bool detach = false)
        {
            if (THREAD_STATUS_STANDBY != getStatus())
            {
                return false;
            }

            setStatus(THREAD_STATUS_RUNNING);
            _thread = std::thread(&Derived::threadMain, static_cast<Derived*>(this));

            if (true == detach)
            {
                _thread.detach();
            }
        }

        void Stop()
        {
            if (THREAD_STATUS_RUNNING == getStatus()
                && true == _thread.joinable())
            {
                setStatus(THREAD_STATUS_TERMINATED);
                _thread.join();
            }
        }

        Thread() = default;
        
        virtual ~Thread()
        {
            Stop();
        }

    protected:
        inline void setStatus(ThreadStatus status)
        {
            _status.store(status, std::memory_order_acquire);
        }

        inline ThreadStatus getStatus()
        {
            return _status.load(std::memory_order_acquire);
        }

        // inline void yield()
        // {
        //     std::this_thread::yield();
        // }

    private:
        std::atomic<ThreadStatus>       _status { THREAD_STATUS_STANDBY };
        std::thread                     _thread;
    };
}
}

#endif // __ZS_COMMON_THREAD_H__
