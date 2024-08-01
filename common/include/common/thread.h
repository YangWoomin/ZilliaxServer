
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
        THREAD_STATUS_TERMINATED = 2,
    };

    template <typename Derived>
    class Thread
    {
    public:
        virtual bool Start(bool detach = false)
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

            return true;
        }

        virtual void Stop()
        {
            if (THREAD_STATUS_RUNNING == getStatus())
            {
                setStatus(THREAD_STATUS_TERMINATED);
            }
        }

        virtual void Join()
        {
            if (true == _thread.joinable())
            {
                _thread.join();
            }
        }

        Thread() = default;
        
        virtual ~Thread()
        {
            Stop();
            Join();
        }

    protected:
        inline void setStatus(ThreadStatus status)
        {
            _status.store(status, std::memory_order_seq_cst);
        }

        inline ThreadStatus getStatus()
        {
            return _status.load(std::memory_order_acquire);
        }

    private:
        std::atomic<ThreadStatus>       _status { THREAD_STATUS_STANDBY };
        std::thread                     _thread;

        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;
    };
}
}

#endif // __ZS_COMMON_THREAD_H__
