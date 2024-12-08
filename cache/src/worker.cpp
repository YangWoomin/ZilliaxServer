
#include    "worker.h"
#include    "common/log.h"

using namespace zs::common;
using namespace zs::cache;

void Operation1::Callback()
{
    try 
    {
        auto res = _fut.get();
        if (nullptr != _cb)
        {
            _cb(_cid, std::move(_keys), std::move(_args), true, res ? std::move(*res) : ResultSet1{});
        }
    } 
    catch (const Error& err) 
    {
        ZS_LOG_ERROR(cache, "cache operation failed, cid : %llu, msg : %s",
            _cid, err.what());
        
        if (nullptr != _cb)
        {
            _cb(_cid, std::move(_keys), std::move(_args), false, ResultSet1{});
        }
    }
}

void Operation2::Callback()
{
    try 
    {
        auto res = _fut.get();
        if (nullptr != _cb)
        {
            _cb(_cid, std::move(_keys), std::move(_args), true, res ? *res : 0);
        }
    } 
    catch (const Error& err) 
    {
        ZS_LOG_ERROR(cache, "cache operation failed, cid : %llu, msg : %s",
            _cid, err.what());
        
        if (nullptr != _cb)
        {
            _cb(_cid, std::move(_keys), std::move(_args), false, 0);
        }
    }
}

bool Worker::Post(OperationSPtr op)
{
    if (THREAD_STATUS_RUNNING != getStatus())
    {
        return false;
    }

    bool signal = false;
    {
        _lock.lock();
        signal = _ops.empty();
        _ops.push(op);
        _lock.unlock();
    }
    
    if (true == signal)
    {
        _signal.notify_one();
    }

    return true;
}

void Worker::Stop()
{
    Thread<Worker>::Stop();

    _signal.notify_one();

    Join();
}

void Worker::threadMain()
{
    std::unique_lock<std::mutex> ulock { _lock, std::defer_lock };
    while (THREAD_STATUS_RUNNING == getStatus())
    {
        ulock.lock();

        if (true == _ops.empty())
        {
            _signal.wait(ulock);
        }

        if (false == _ops.empty())
        {
            OperationSPtr op = _ops.front();
            _ops.pop();
            ulock.unlock();
            op->Callback();
        }
        else
        {
            ulock.unlock();
        }
    }

    ulock.lock();
    while (false == _ops.empty())
    {
        OperationSPtr op = _ops.front();
        _ops.pop();
        op->Callback();
    }
    ulock.unlock();
}
