
#include    "msg_worker.h"

using namespace zs::common;
using namespace zs::mq;

const std::string MsgWorker::INVALID_ARGUMENT_FOR_CACHE_SCRIPT = "invalid arguement";

bool MsgWorker::Initialize(int32_t intervalMs, CacheConfig& cacheConfig, ProducerSPtr producer)
{
    if (0 < intervalMs)
    {
        // _intervalMs is 10 by default
        _intervalMs = intervalMs;
    }

    _storedMsgSnTmpListTtlSec = cacheConfig._storedMsgSnTmpListTtlSec;
    _storedMsgSnTmpListMaxCount = cacheConfig._storedMsgSnTmpListMaxCount;
    _sendingMsgLoadCount = cacheConfig._sendingMsgLoadCount;

    _producer = producer;
    
    return true;
}

void MsgWorker::Finalize()
{
    Stop();
    Join();

    _producer.reset();
    _clients.clear();
}

void MsgWorker::AddClient(const char* client)
{
    std::lock_guard<std::mutex> lock(_mtx);
    _reqs.push({client, true});
}

void MsgWorker::RemoveClient(const char* client)
{
    std::lock_guard<std::mutex> lock(_mtx);
    _reqs.push({client, false});
}

void MsgWorker::HandleProducedMessage(MessageStatus status, Message* msg, const std::string& err, ClientSPtr client)
{
    if (MESSAGESTATUS_NOT_PERSISTED == status)
    {
        ZS_LOG_WARN(mq_test_producer, "something wrong in mq producer, key : %s, sn : %llu, err msg : %s",
            msg ? msg->_key.c_str() : "", msg ? msg->_sn : 0, err.c_str());
        
        // update missed msg sn
        std::lock_guard<std::mutex> lock(client->_mtx);
        if (0 < client->_ms._missedSn)
        {
            client->_ms._missedSn = std::min(client->_ms._missedSn, msg->_sn);
        }
        else
        {
            client->_ms._missedSn = msg->_sn;
        }
    }
    else if (MESSAGESTATUS_PERSISTED == status ||  MESSAGESTATUS_POSSIBLY_PERSISTED == status)
    {
        // ZS_LOG_INFO(mq_test_producer, "message stored in mq producer, key : %s, sn : %llu",
        //     msg->_key.c_str(), msg->_sn);

        // update stored msg sn
        std::lock_guard<std::mutex> lock(client->_mtx);
        client->_ms._storedMsgSn.push_back(msg->_sn);
    }
    else if (MESSAGESTATUS_MISSING_TEST == status)
    {
        ZS_LOG_WARN(mq_test_producer, "missing message test occurred in mq producer, key : %s, sn : %llu",
            msg->_key.c_str(), msg->_sn);
    }

    delete msg;
}

void MsgWorker::threadMain()
{
    while (THREAD_STATUS_RUNNING == getStatus())
    {
        // handle client connection requests
        std::queue<ClientConnectionRequest> q;
        {
            std::lock_guard<std::mutex> lock(_mtx);
            q.swap(_reqs);
        }
        while (false == q.empty())
        {
            auto& req = q.front();
            if (true == req._connected)
            {
                _clients[req._clientID] = std::make_shared<Client>();
                _clients[req._clientID]->_init = false;
                _clients[req._clientID]->_worker = shared_from_this();
            }
            else
            {
                _clients.erase(req._clientID);
            }
            q.pop();
        }

        // handle client messages
        handleClientMsgStatus();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(_intervalMs));
    }
}

void MsgWorker::handleClientMsgStatus()
{
    for (auto& [clientId, client] : _clients)
    {
        ClientMsgStatus status;
        status._clientID = clientId;
        status._sendingMsgCount = client->_ms._sendingMsgCount;
        client->_ms._sendingMsgCount = 0;
        {
            std::lock_guard<std::mutex> lock(client->_mtx);
            
            status._missedSn = !client->_init ? MsgWorker::CACHE_MSG_SN_INIT : client->_ms._missedSn;
            client->_init = true;
            client->_ms._missedSn = 0;

            status._storedMsgSn = std::move(client->_ms._storedMsgSn);
        }

        ResultSet1 sendingMsgSn;
        loadMessage(status._clientID, status._missedSn, status._sendingMsgCount, status._storedMsgSn, sendingMsgSn);

        std::string res = 0 < sendingMsgSn.size() ? sendingMsgSn[0] : "0";
        if (INVALID_ARGUMENT_FOR_CACHE_SCRIPT == res)
        {
            ZS_LOG_ERROR(mq_test_producer, "invalid argument for loading message, client : %s",
                clientId.c_str());
        }
        else
        {
            MsgSN sn = std::stoull(res);
            for (std::size_t i = 1; i < sendingMsgSn.size() && 0 < sn; ++i)
            {
                auto cid = clientId;
                if (true == produceMessage(cid, sendingMsgSn[i], 
                    MsgHeaders{
                        {"sn", std::to_string(sn)},
                        {"cid", cid}
                    },
                    sn, client))
                {
                    client->_ms._sendingMsgCount++;
                    sn++;
                }
                else
                {
                    break;
                }
            }
        }
    }
}

void MsgWorker::loadMessage(const std::string& clientId, MsgSN missedSn, int32_t sendingMsgCount, const std::vector<MsgSN>& storedMsgSn, ResultSet1& sendingMsgSn)
{
// we want schema.. T.T
    static Script script = R"(

if (#KEYS ~= 1 or #ARGV < 6) then return {"invalid arguement"} end
local key = "producer:" .. KEYS[1]
local stored_msg_sn_tmp_list_ttl = tonumber(ARGV[1])
local stored_msg_sn_tmp_list_max_count = tonumber(ARGV[2])
local sending_msg_load_count = tonumber(ARGV[3])
local missed_sn = tonumber(ARGV[4])
local sending_msg_sn_cnt = tonumber(ARGV[5])
local stored_msg_sn_cnt = tonumber(ARGV[6])
local stored_msg_sn_start_idx = 7
local stored_msg_sn_key = key .. ":stored_msg_sn"
local sending_msg_sn_key = key .. ":sending_msg_sn"
local stored_msg_sn_tmp_list_key = key .. ":stored_msg_sn_tmp_list"
local stored_msg_sn_tmp_list_timer_key = key .. ":stored_msg_sn_tmp_list_timer"

local function reset(key, stored_msg_sn)
    redis.call("SET", stored_msg_sn_key, stored_msg_sn)
    redis.call("SET", sending_msg_sn_key, stored_msg_sn)
    redis.call("DEL", stored_msg_sn_tmp_list_key)
end

-- update sending message sn
if (0 < sending_msg_sn_cnt) then
    redis.call("INCRBY", sending_msg_sn_key, sending_msg_sn_cnt)
end

-- update stored message sn
for i = stored_msg_sn_start_idx, stored_msg_sn_start_idx - 1 + stored_msg_sn_cnt do
    local sn = tonumber(ARGV[i])
    redis.call("ZADD", stored_msg_sn_tmp_list_key, sn, sn)
end
if (stored_msg_sn_cnt > 0) then
    redis.call("SET", stored_msg_sn_tmp_list_timer_key, 1, "EX", stored_msg_sn_tmp_list_ttl)
end
local stored_msg_sn = tonumber(redis.call("GET", stored_msg_sn_key) or '0')
local stored_msg_sn_tmp_list = redis.call("ZRANGEBYSCORE", stored_msg_sn_tmp_list_key, "-inf", "+inf", "WITHSCORES")
for i = 1, #stored_msg_sn_tmp_list, 2 do
    local sn = tonumber(stored_msg_sn_tmp_list[i])
    if (sn == stored_msg_sn + 1) then
        stored_msg_sn = stored_msg_sn + 1
        redis.call("ZREM", stored_msg_sn_tmp_list_key, sn)
    else
        break
    end
end
redis.call("SET", stored_msg_sn_key, stored_msg_sn)

-- check missed sn
if (-1 == missed_sn) then
    -- initialization
    reset(key, stored_msg_sn)
elseif (0 < missed_sn) then
    -- missed message
    local sending_msg_sn = tonumber(redis.call("GET", sending_msg_sn_key) or '0')
    if (stored_msg_sn >= missed_sn) then
        reset(key, missed_sn - 1)
    elseif (sending_msg_sn >= missed_sn) then
        redis.call("SET", sending_msg_sn_key, missed_sn - 1)
    end
else 
    -- 0 means no missed messages
    -- check stored_msg_sn_tmp_list
    local tmp_stored_sn_count = redis.call("ZCARD", stored_msg_sn_tmp_list_key)
    local tmp_stored_sn_list_ttl = tonumber(redis.call("GET", stored_msg_sn_tmp_list_timer_key) or '0')
    if (stored_msg_sn_tmp_list_max_count < tmp_stored_sn_count or (0 < tmp_stored_sn_count and 0 == tmp_stored_sn_list_ttl)) then
        reset(key, stored_msg_sn)
    end
end

-- load messages to send
local msgs = {}
local next_msg_sn = tonumber(redis.call("GET", sending_msg_sn_key) or '0') + 1
for i = 1, sending_msg_load_count do
    local next_msg = redis.call("GET", key .. ":" .. tostring(next_msg_sn)) or ""
    if ("" == next_msg) then
        break
    else
        if i == 1 then
            table.insert(msgs, tostring(next_msg_sn))
        end
        table.insert(msgs, next_msg)
        next_msg_sn = next_msg_sn + 1
    end
end

return msgs

    )";

    Keys keys = {clientId};
    Args args = {
        std::to_string(_storedMsgSnTmpListTtlSec),
        std::to_string(_storedMsgSnTmpListMaxCount),
        std::to_string(_sendingMsgLoadCount),
        std::to_string(missedSn),
        std::to_string(sendingMsgCount),
        std::to_string(storedMsgSn.size())
    };

    for (const auto& storedMsgSN : storedMsgSn)
    {
        args.push_back(std::to_string(storedMsgSN));
    }

    // sync
    while (false == Cache::Run(script, ++_cid, std::move(keys), std::move(args), sendingMsgSn))
    {
        ZS_LOG_ERROR(mq_test_producer, "loading message failed, retrying, client : %s, msg : %s, cid : %llu",
            clientId.c_str(), args[0].c_str(), _cid);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool MsgWorker::produceMessage(const std::string& key, const std::string& payload, MsgHeaders&& headers, uint64_t sn, ClientSPtr client)
{
    if (nullptr != _producer)
    {
        Message* msg = new Message();
        msg->_key = key;
        msg->_buf = payload;
        msg->_headers = std::move(headers);
        msg->_sn = sn;
        msg->_ctx = client;
        return _producer->Produce(msg);
    }
    
    ZS_LOG_ERROR(mq_test_producer, "producing message failed, key : %s, sn : %llu",
        key, sn);
    
    return false;
}
