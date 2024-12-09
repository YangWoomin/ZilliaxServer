
#include    "common/types.h"
#include    "common/thread.h"

#include    "mq/mq.h"
#include    "cache/cache.h"
#include    "msg_manager.h"

#include    <memory>
#include    <mutex>
#include    <unordered_map>
#include    <queue>

using namespace zs::common;
using namespace zs::mq;

class MsgWorker;
using MsgWorkerWPtr = std::weak_ptr<MsgWorker>;

struct ClientMsgStatus
{
    std::string         _clientID;
    MsgSN               _missedSn = 0;
    std::vector<MsgSN>  _storedMsgSn;

    int32_t             _sendingMsgCount = 0; // no lock needed
};

struct Client : public MessageContext
{
    bool            _init = false;
    MsgWorkerWPtr   _worker;

    std::mutex      _mtx;
    ClientMsgStatus _ms;

    virtual ~Client() = default;
};
using ClientSPtr = std::shared_ptr<Client>;

struct ClientConnectionRequest
{
    std::string     _clientID;
    bool            _connected;
};

class MsgWorker final : public Thread<MsgWorker>
    , public std::enable_shared_from_this<MsgWorker>
{
public:
    bool Initialize(int32_t intervalMs, CacheConfig& cacheConfig, ProducerSPtr producer);
    void Finalize();

    void AddClient(const char* client);
    void RemoveClient(const char* client);

    void HandleProducedMessage(MessageStatus status, Message* msg, const std::string& err, ClientSPtr client);

    MsgWorker() = default;
    ~MsgWorker() = default;

private:
    int32_t                                     _intervalMs = 10;
    int32_t                                     _storedMsgSnTmpListTtlSec;
    int32_t                                     _storedMsgSnTmpListMaxCount;
    int32_t                                     _sendingMsgLoadCount;
    std::mutex                                  _mtx;
    std::queue<ClientConnectionRequest>         _reqs;
    std::unordered_map<std::string, ClientSPtr> _clients;
    ContextID                                   _cid = 0;
    ProducerSPtr                                _producer;

    void threadMain();

    void handleClientMsgStatus();
    void loadMessage(const std::string& clientId, MsgSN missedSn, int32_t sendingMsgCount, const std::vector<MsgSN>& storedMsgSn, ResultSet1& sendingMsgSn);
    bool produceMessage(const std::string& key, const std::string& payload, MsgHeaders&& headers, uint64_t sn, ClientSPtr client);

    static const int32_t CACHE_MSG_SN_INIT = -1;
    static const std::string INVALID_ARGUMENT_FOR_CACHE_SCRIPT;

friend class Thread<MsgWorker>;
};
