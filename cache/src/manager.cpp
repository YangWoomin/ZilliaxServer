
#include    "cache/cache.h"

#include    "common/log.h"
#include    "worker.h"

#include    "manager.h"
#include    "worker.h"

using namespace zs::common;
using namespace zs::cache;
using namespace sw::redis;

bool Manager::Initialize(const std::string& dsn, int32_t workerCount)
{
    if (nullptr != _arc)
    {
        ZS_LOG_ERROR(cache, "cache manager already initialized");
        return false;
    }

    if (0 >= workerCount)
    {
        ZS_LOG_ERROR(cache, "invalid worker count in cache manager, count : %d",
            workerCount);
        return false;
    }

    for (auto i = 0; i < workerCount; ++i)
    {
        WorkerSPtr worker = std::make_shared<Worker>();
        if (false == worker->Start())
        {
            ZS_LOG_ERROR(cache, "creating cache worker failed, idx : %d",
                i);
            return false;
        }
        _workers.push_back(worker);
    }

    try 
    {
        _arc = std::make_shared<AsyncRedisCluster>(dsn);
        _rc = std::make_shared<RedisCluster>(dsn);
    } 
    catch (const Error &err) 
    {
        ZS_LOG_ERROR(cache, "creating cache handles failed, msg : %s",
            err.what());
        return false;
    }

    ZS_LOG_INFO(cache, "cache manager initialized");

    return true;
}

void Manager::Finalize()
{
    for (auto& worker : _workers)
    {
        worker->Stop();
    }
    _workers.clear();

    try 
    {
        _arc.reset();
        _rc.reset();
    } 
    catch (const Error &err) 
    {
        ZS_LOG_WARN(cache, "clearing cache handles failed, msg : %s",
            err.what());
    }
}

bool Manager::Run(const Script& script, ContextID cid, Keys&& keys, Args&& args, WorkerHash wh, AsyncSet1Callback cb)
{
    if (nullptr == _arc)
    {
        ZS_LOG_ERROR(cache, "cache manager not initialized");
        return false;
    }

    Keys hKeys;
    for (auto key : keys)
    {
        hKeys.push_back("{" + key + "}");
    }

    Operation1SPtr op = std::make_shared<Operation1>();
    op->_script = script;
    op->_cid = cid;
    op->_keys = std::move(keys);
    op->_args = std::move(args);
    op->_cb = cb;
    op->_fut = _arc->eval<Optional<ResultSet1>>(
        op->_script, 
        hKeys.begin(), 
        hKeys.end(), 
        op->_args.begin(), 
        op->_args.end()
    );

    return _workers[wh % _workers.size()]->Post(op);
}

bool Manager::Run(const Script& script, ContextID cid, Keys&& keys, Args&& args, WorkerHash wh, AsyncSet2Callback cb)
{
    if (nullptr == _arc)
    {
        ZS_LOG_ERROR(cache, "cache manager not initialized");
        return false;
    }

    Keys hKeys;
    for (auto key : keys)
    {
        hKeys.push_back("{" + key + "}");
    }

    Operation2SPtr op = std::make_shared<Operation2>();
    op->_script = script;
    op->_cid = cid;
    op->_keys = std::move(keys);
    op->_args = std::move(args);
    op->_cb = cb;
    op->_fut = _arc->eval<Optional<SimpleResult>>(
        op->_script, 
        hKeys.begin(), 
        hKeys.end(), 
        op->_args.begin(), 
        op->_args.end()
    );

    return _workers[wh % _workers.size()]->Post(op);
}

bool Manager::Run(const Script& script, ContextID cid, Keys&& keys, Args&& args, ResultSet1& rs)
{
    if (nullptr == _rc)
    {
        ZS_LOG_ERROR(cache, "cache manager not initialized");
        return false;
    }

    Keys hKeys;
    for (auto key : keys)
    {
        hKeys.push_back("{" + key + "}");
    }

    try 
    {
        rs = _rc->eval<ResultSet1>(script, hKeys.begin(), hKeys.end(), args.begin(), args.end());
    } 
    catch (const Error& err) 
    {
        ZS_LOG_ERROR(cache, "cache operation failed, cid : %llu, msg : %s",
            cid, err.what());
        return false;
    }

    return true;
}

bool Manager::Run(const Script& script, ContextID cid, Keys&& keys, Args&& args, SimpleResult& rs)
{
    if (nullptr == _rc)
    {
        ZS_LOG_ERROR(cache, "cache manager not initialized");
        return false;
    }

    Keys hKeys;
    for (auto key : keys)
    {
        hKeys.push_back("{" + key + "}");
    }

    try 
    {
        rs = _rc->eval<SimpleResult>(script, hKeys.begin(), hKeys.end(), args.begin(), args.end());
    } 
    catch (const Error& err) 
    {
        ZS_LOG_ERROR(cache, "cache operation failed, cid : %llu, msg : %s",
            cid, err.what());
        return false;
    }

    return true;
}
