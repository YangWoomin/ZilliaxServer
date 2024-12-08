#pragma once

#include    "common/types.h"
#include    "common/log.h"

#include    "cache/cache.h"
#include    "mq/mq.h"

#include    "mq_producer.h"

#include    <atomic>

using namespace zs::common;
using namespace zs::cache;
using namespace zs::mq;

class MsgManager final
{
public:
    bool Initialize(Logger::Messenger msgr, int32_t workerCount, const std::string& cacheAddr, int32_t cacheWorkerCount, const std::string& mqAddr, const std::string& debug, const std::string& topic, int32_t mqPollerCount, int32_t mqPollingIntervalMs);
    void Finalize();

    bool StoreMessage(const char* client, const char* msg, std::size_t len);

private:
    std::shared_ptr<MQProducer>     _mp;
    std::atomic<bool>               _run {false};
    std::atomic<ContextID>          _cid {0};

    static void handleStoredMessage(ContextID cid, Keys&& keys, Args&& args, bool success, SimpleResult res);

    static Script MESSAGE_STORE_SCRIPT;
};
