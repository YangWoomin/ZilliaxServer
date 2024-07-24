
#include    "db/database.h"
#include    "worker.h"

using namespace zs::common;
using namespace zs::db;

bool Database::Initialize(const Config& config, Logger::Messenger msgr)
{
    Logger::Messenger& messenger = Logger::GetMessenger();
    messenger = msgr;

    if (true == _init)
    {
        ZS_LOG_ERROR(db, "db already initialized");
        return false;
    }

    if (0 >= config._workerCnt)
    {
        ZS_LOG_ERROR(db, "invalid db worker count");
        return false;
    }

    for (std::size_t i = 0; i < config._workerCnt; ++i)
    {
        WorkerSPtr worker = std::make_shared<Worker>();
        if (false == worker->Initialize(config))
        {
            ZS_LOG_ERROR(db, "%lu db worker init failed", i);
            return false;
        }
        _workers.push_back(worker);
    }

    _init = true;

    ZS_LOG_INFO(db, "db initialized");

    return true;
}

void Database::Finalize()
{
    _workers.clear();

    _init = false;
    
    ZS_LOG_INFO(db, "db finalized");
}

bool Database::Start()
{
    if (false == _init)
    {
        ZS_LOG_ERROR(db, "db initialized not yet");
        return false;
    }

    for (std::size_t i = 0; i < _workers.size(); ++i)
    {
        if (false == _workers[i]->Start())
        {
            ZS_LOG_ERROR(db, "%lu db worker start failed", i);
            return false;
        }
    }

    return true;
}

void Database::Stop()
{
    for (std::size_t i = 0; i < _workers.size(); ++i)
    {
         _workers[i]->Stop();
    }
}

bool Database::Post(std::size_t workerNum, OperationSPtr op)
{
    if (false == _init)
    {
        ZS_LOG_ERROR(db, "db initialized not yet");
        return false;
    }

    std::size_t hash = workerNum % _workers.size();
    return _workers[hash]->Post(op);
}
