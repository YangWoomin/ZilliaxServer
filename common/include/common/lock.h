
#ifndef __ZS_COMMON_LOCK_H__
#define __ZS_COMMON_LOCK_H__

#if defined(_WIN64_)
#include    <Windows.h>
#else // defined(_WIN64_)
#include    <mutex>
#endif // defined(_WIN64_)

namespace zs
{
namespace common
{
    class Lock
    {
    public:
        Lock()
        {
#if defined(_WIN64_)
            InitializeCriticalSection(&_lock);
#endif // defined(_WIN64_)
        }

        ~Lock()
        {
#if defined(_WIN64_)
            DeleteCriticalSection(&_lock);
#endif // defined(_WIN64_)
        }

        void DoLock()
        {
#if defined(_WIN64_)
            EnterCriticalSection(&_lock);
#else // defined(_WIN64_)
            _lock.lock();
#endif // defined(_WIN64_)
        }

        void Unlock()
        {
#if defined(_WIN64_)
            LeaveCriticalSection(&_lock);
#else // defined(_WIN64_)
            _lock.unlock();
#endif // defined(_WIN64_)
        }

    private:
#if defined(_WIN64_)
        CRITICAL_SECTION        _lock;
#else // defined(_WIN64_)
        std::recursive_mutex    _lock;
#endif // defined(_WIN64_)
    };

    class AutoScopeLock
    {
    public:
        AutoScopeLock(Lock* lock)
            : _lock(lock)
        {
            if (nullptr != _lock)
            {
                _lock->DoLock();
            }
        }

        ~AutoScopeLock()
        {
            if (nullptr != _lock)
            {
                _lock->Unlock();
            }
        }

    private:
        Lock*                   _lock = nullptr;
    };
}
}

#endif // __ZS_COMMON_LOCK_H__
