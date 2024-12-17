
#include    "msg_manager.h"
#include    "msg_worker.h"

using namespace zs::common;
using namespace zs::cache;
using namespace zs::mq;

static std::atomic<uint64_t> sentCount {0};

bool MsgManager::Initialize(Logger::Messenger msgr, int32_t workerCount, int32_t workerIntervalMs, CacheConfig&& cacheConfig, MQConfig&& mqConfig)
{
    if (true == _run)
    {
        ZS_LOG_ERROR(mq_test_producer, "msg manager already initialized");
        return false;
    }

    if (0 >= workerCount)
    {
        ZS_LOG_ERROR(mq_test_producer, "invalid worker count for msg manager, count : %d",
            workerCount);
        return false;
    }

    // cache
    if (false == Cache::Initialize(msgr, cacheConfig._addr, cacheConfig._workerCount))
    {
        ZS_LOG_ERROR(mq_test_producer, "cache init failed in msg manager, addr : %s, worker count : %d",
            cacheConfig._addr.c_str(), cacheConfig._workerCount);
        return false;
    }

    // mq
    mqConfig._configs.push_back({"debug", mqConfig._debug});
    mqConfig._configs.push_back({"metadata.broker.list", mqConfig._addr});

    auto eventCallback = [](EventType type, LogLevel level, const std::string& msg) {
        if (LOGLEVEL_FATAL == level)
        {
            ZS_LOG_FATAL(mq_test_producer, "fatal error from mq, msg : %s",
                msg.c_str());
        }
        else
        {
            ZS_LOG(mq_test_producer, level, "%s", msg.c_str());
        }
    };

    auto producingCallback = [](MessageStatus status, Message* msg, const std::string& err) {
        
        auto client = std::dynamic_pointer_cast<Client>(msg->_ctx);
        if (nullptr != client)
        {
            MsgWorkerSPtr worker = client->_worker.lock();
            if (nullptr != worker)
            {
                worker->HandleProducedMessage(status, msg, err, client);
                return;
            }
        }
        
        ZS_LOG_ERROR(mq_test_producer, "trying to handling produced msg failed, key : %s, sn : %llu, err : %s",
            msg->_key.c_str(), msg->_sn, err.c_str());

        delete msg;
    };

    if (false == MQ::Initialize(msgr, mqConfig._configs, eventCallback, producingCallback, mqConfig._pollerCount, mqConfig._pollingTimeoutMs, mqConfig._pollingIntervalMs))
    {
        ZS_LOG_ERROR(mq_test_producer, "initializing mq module failed");
        return false;
    }

    ProducerSPtr producer = MQ::CreateProducer(mqConfig._topic);
    if (nullptr == producer)
    {
        ZS_LOG_ERROR(mq_test_producer, "creating mq producer failed");
        return false;
    }
    
    // workers
    for (auto i = 0; i < workerCount; ++i)
    {
        MsgWorkerSPtr worker = std::make_shared<MsgWorker>();
        if (false == worker->Initialize(workerIntervalMs, cacheConfig, producer)
            || false == worker->Start())
        {
            ZS_LOG_ERROR(mq_test_producer, "starting msg worker failed, idx : %d",
                i);
            return false;
        }
        _workers.push_back(worker);
    }

    _run = true;
    _cacheConfig = std::move(cacheConfig);
    _mqConfig = std::move(mqConfig);
    _producer = producer;

    ZS_LOG_INFO(mq_test_producer, "msg manager initialized");

    return true;
}

void MsgManager::Finalize()
{
    for (auto& worker : _workers)
    {
        worker->Finalize();
    }
    _workers.clear();

    _producer.reset();

    MQ::Finalize();
    Cache::Finalize();

    _run = false;

    ZS_LOG_INFO(mq_test_producer, "msg manager finalized, total sent msg count : %llu",
        sentCount.load());
}

void MsgManager::AddClient(const char* client)
{
    if (0 < _workers.size())
    {
        uint64_t hash = std::hash<std::string>{}(client);
        _workers[hash % _workers.size()]->AddClient(client);
    }
}

void MsgManager::RemoveClient(const char* client)
{
    for (auto& worker : _workers)
    {
        worker->RemoveClient(client);
    }
}

bool MsgManager::StoreMessage(const char* client, const char* msg, std::size_t len)
{
    if (false == _run)
    {
        return false;
    }

    static const Script script = R"(

if (#KEYS ~= 1 or #ARGV ~= 2) then return 0 end
local key = "producer:" .. KEYS[1]
local msg = ARGV[1]
local ttl = tonumber(ARGV[2])
local next_sn_key = key .. ":next_sn"
local next_sn = redis.call("INCR", next_sn_key)
redis.call("SET", key .. ":" .. tostring(next_sn), msg, "EX", ttl)
return next_sn

    )";

    std::string clientId(client);
    Keys keys = {clientId};
    Args args = {std::string(msg, len), std::to_string(_cacheConfig._msgTtl)};
    WorkerHash wh = std::hash<std::string>{}(clientId);
    ContextID cid = ++_cid;
    AsyncSet2Callback cb = MsgManager::handleStoredMessage;

    // async
    while (false == Cache::Run(script, cid, std::move(keys), std::move(args), wh, cb))
    {
        ZS_LOG_ERROR(mq_test_producer, "caching message failed, retrying, client : %s, msg : %s, cid : %llu",
            clientId.c_str(), args[0].c_str(), cid);
    }

    return true;
}

MsgManager::~MsgManager()
{
    Finalize();
}

void MsgManager::handleStoredMessage(ContextID cid, Keys&& keys, Args&& args, bool success, SimpleResult res)
{
    if (success)
    {
        // ZS_LOG_INFO(mq_test_producer, "caching message succeeded, client : %s, cid : %llu, res : %d",
        //     keys[0].c_str(), cid, res);
        
        sentCount++;
    }
    else
    {
        // this could make the messages lost and unordered
        // server should notify client that the message was lost
        // and induce client to send the message again
        // but now we just log as todo
        ZS_LOG_ERROR(mq_test_producer, "caching message failed, client : %s, msg : %s, cid : %llu",
            keys[0].c_str(), args[0].c_str(), cid);
    }
}
