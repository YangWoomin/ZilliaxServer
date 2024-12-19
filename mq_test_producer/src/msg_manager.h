#pragma once

#include    "common/types.h"
#include    "common/log.h"

#include    "cache/cache.h"
#include    "mq/mq.h"

#include    <atomic>
#include    <unordered_map>

using namespace zs::common;
using namespace zs::cache;
using namespace zs::mq;

struct CacheConfig
{
    std::string _addr;
    int32_t _workerCount;
    int32_t _msgTtl;
    int32_t _storedMsgSnTmpListTtlSec;
    int32_t _storedMsgSnTmpListMaxCount;
    int32_t _sendingMsgLoadCount;
};

struct MQConfig
{
    std::string _addr;
    std::string _debug;
    std::string _topic;
    int32_t _pollerCount;
    int32_t _pollingIntervalMs;
    int32_t _pollingTimeoutMs;
    ConfigList _configs;
};

class MsgWorker;
using MsgWorkerSPtr = std::shared_ptr<MsgWorker>;

class MsgCounter;
using MsgCounterSPtr = std::shared_ptr<MsgCounter>;

using MsgSN = uint64_t; // message sequence/serial number
using MsgHeaders = std::unordered_map<std::string, std::string>;

class MsgManager final
{
public:
    bool Initialize(Logger::Messenger msgr, int32_t workerCount, int32_t workerIntervalMs, CacheConfig&& cacheConfig, MQConfig&& mqConfig);
    void Finalize();

    void AddClient(const char* client);
    void RemoveClient(const char* client);

    bool StoreMessage(const char* client, const char* msg, std::size_t len);

    ~MsgManager();

private:
    std::atomic<bool>               _run {false};
    std::atomic<ContextID>          _cid {0};
    CacheConfig                     _cacheConfig;
    MQConfig                        _mqConfig;
    std::vector<MsgWorkerSPtr>      _workers;
    ProducerSPtr                    _producer;
    MsgCounterSPtr                  _counter;

    static void handleStoredMessage(ContextID cid, Keys&& keys, Args&& args, bool success, SimpleResult res);

friend class MsgWorker;
};
