
#include    "msg_manager.h"

using namespace zs::common;
using namespace zs::cache;
using namespace zs::mq;

static std::atomic<uint64_t> sentCount {0};

Script MsgManager::MESSAGE_STORE_SCRIPT = R"(
    if (#KEYS ~= 1 or #ARGV ~= 1) then return 0 end
    local key = KEYS[1]
    local msg = ARGV[1]
    local stored_sn_key = key .. ":stored_sn"
    local sending_sn_key = key .. ":sending_sn"
    local stored_sn = tonumber(redis.call("GET", stored_sn_key) or '0')
    local sending_sn = tonumber(redis.call("GET", sending_sn_key) or tostring(stored_sn))
    local next_sending_sn = redis.call("INCR", sending_sn_key)
    redis.call("SET", key .. ":" .. tostring(next_sending_sn), msg, "EX", 3000)
    return next_sending_sn
)";

bool MsgManager::Initialize(Logger::Messenger msgr, int32_t workerCount, const std::string& cacheAddr, int32_t cacheWorkerCount, const std::string& mqAddr, const std::string& debug, const std::string& topic, int32_t mqPollerCount, int32_t mqPollingIntervalMs)
{
    if (false == Cache::Initialize(msgr, cacheAddr, cacheWorkerCount))
    {
        ZS_LOG_ERROR(mq_test_producer, "cache init failed in msg manager, addr : %s, worker count : %d",
            cacheAddr.c_str(), cacheWorkerCount);
        return false;
    }

    std::shared_ptr<MQProducer> mp = std::make_shared<MQProducer>();
    if (false == mp->Initialize(msgr, mqAddr, debug, mqPollerCount, mqPollingIntervalMs))
    {
        ZS_LOG_ERROR(mq_test_producer, "mq init failed in msg manager, addr : %s, worker count : %d, topic : %s",
            mqAddr.c_str(), mqPollerCount, topic.c_str());
        return false;
    }
    
    if (false == mp->CreateProducer(topic))
    {
        ZS_LOG_ERROR(mq_test_producer, "creating mq producer failed in msg manager, addr : %s, topic : %s",
            mqAddr.c_str(), topic.c_str());
        return false;
    }

    _mp = mp;
    _run = true;

    ZS_LOG_INFO(mq_test_producer, "msg manager initialized");

    return true;
}

void MsgManager::Finalize()
{
    _mp.reset();
    Cache::Finalize();

    ZS_LOG_INFO(mq_test_producer, "msg manager finalized, total sent msg count : %llu",
        sentCount.load());
}

bool MsgManager::StoreMessage(const char* client, const char* msg, std::size_t len)
{
    if (false == _run)
    {
        return false;
    }

    std::string clientId(client);
    Keys keys = {clientId};
    Args args = {std::string(msg, len)};
    WorkerHash wh = std::hash<std::string>{}(clientId);
    ContextID cid = ++_cid;
    AsyncSet2Callback cb = MsgManager::handleStoredMessage;

    while (false == Cache::Set(MESSAGE_STORE_SCRIPT, cid, std::move(keys), std::move(args), wh, cb))
    {
        ZS_LOG_ERROR(mq_test_producer, "caching message failed 2, retrying, client : %s, msg : %s, cid : %llu",
            clientId.c_str(), args[0].c_str(), cid);
        return false;
    }


    return true;
}

void MsgManager::handleStoredMessage(ContextID cid, Keys&& keys, Args&& args, bool success, SimpleResult res)
{
    if (success)
    {
        ZS_LOG_INFO(mq_test_producer, "caching message succeeded, client : %s, cid : %llu, res : %d",
            keys[0].c_str(), cid, res);
        
        sentCount++;
    }
    else
    {
        ZS_LOG_WARN(mq_test_producer, "caching message failed 1, retrying, client : %s, msg : %s, cid : %llu",
            keys[0].c_str(), args[0].c_str(), cid);
        
        // counting for resending?
        WorkerHash wh = std::hash<std::string>{}(keys[0]);
        Cache::Set(MESSAGE_STORE_SCRIPT, cid, std::move(keys), std::move(args), wh, MsgManager::handleStoredMessage);
    }
}

