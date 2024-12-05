
#include    "mq_producer.h"

using namespace zs::common;
using namespace zs::mq;


bool MQProducer::Initialize(Logger::Messenger msgr, const std::string servers, const std::string debug)
{
    ConfigList configs;
    configs.push_back({"debug", debug});
    configs.push_back({"metadata.broker.list", servers});

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
        if (!err.empty())
        {
            ZS_LOG_ERROR(mq_test_producer, "something wrong in mq producer, status : %d, msg : %s",
                status, err.c_str());
        }
        else
        {
            ZS_LOG_INFO(mq_test_producer, "message delivered in mq producer, key : %s, sn : %llu",
                msg->_key.c_str(), msg->_sn);
        }

        delete msg;
    };

    if (false == MQ::Initialize(msgr, configs, eventCallback, producingCallback))
    {
        return false;
    }

    return true;
}

void MQProducer::Finalize()
{
    MQ::Finalize();
}

bool MQProducer::CreateProducer(std::string topic)
{
    ConfigList configs;

    _producer = MQ::CreateProducer(topic, configs);
    if (nullptr == _producer)
    {
        return false;
    }

    return true;
}

bool MQProducer::Produce(const char* key, const char* payload, std::size_t len, uint64_t sn)
{
    if (nullptr != _producer)
    {
        Message* msg = new Message();
        msg->_key = key;
        msg->_buf = std::string(payload, len);
        msg->_sn = sn;
        return _producer->Produce(msg);
    }
    
    ZS_LOG_ERROR(mq_test_producer, "producing message failed, key : %s, sn : %llu",
        key, sn);
    
    return false;
}

MQProducer::~MQProducer()
{
    Finalize();
}
