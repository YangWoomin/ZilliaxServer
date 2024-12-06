
#include    "mq/mq.h"

#include    "common/log.h"

#include    "producer.h"

using namespace zs::common;
using namespace zs::mq;

bool InternalProducer::Initialize(const std::string& topic_str, RdKafka::Conf* gConfig, const ConfigList& configs)
{
    if (nullptr != _conf)
    {
        ZS_LOG_ERROR(mq, "mq producer already initialized, topic : %s",
            topic_str.c_str());
        return false;
    }

    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);

    std::string errMsg;

    for (const auto& [key, value] : configs)
    {
        if (RdKafka::Conf::CONF_OK != conf->set(key, value, errMsg))
        {
            ZS_LOG_ERROR(mq, "setting config in mq producer failed, topic : %s, key : %s, value : %s, msg : %s", 
                topic_str.c_str(), key.c_str(), value.c_str(), errMsg.c_str());
            delete conf;
            return false;
        }
    }

    RdKafka::Producer* producer = RdKafka::Producer::create(gConfig, errMsg);
    if (nullptr == producer)
    {
        ZS_LOG_ERROR(mq, "creating new producer in mq producer failed, topic : %s, msg : %s", 
            topic_str.c_str(), errMsg.c_str());
        delete conf;
        return false;
    }

    RdKafka::Topic* topic = RdKafka::Topic::create(producer, topic_str, conf, errMsg);
    if (nullptr == topic)
    {
        ZS_LOG_ERROR(mq, "creating new topic in mq producer failed, topic : %s, msg : %s",
            topic_str.c_str(), errMsg.c_str());
        delete conf;
        delete producer;
        return false;
    }

    _conf = conf;
    _producer = producer;
    _topic = topic;

    ZS_LOG_INFO(mq, "mq producer initialized, topic : %s",
        topic_str.c_str());

    return true;
}

void InternalProducer::Finalize()
{
    if (nullptr != _producer)
    {
        while (_producer->outq_len())
        {
            ZS_LOG_INFO(mq, "mq producer is waiting for , topic : %s",
                _topic->name().c_str());

            _producer->poll(1000);
        }

        delete _producer;
        _producer = nullptr;
    }

    if (nullptr != _topic)
    {
        delete _topic;
        _topic = nullptr;
    }

    if (nullptr != _conf)
    {
        delete _conf;
        _conf = nullptr;
    }
}

bool InternalProducer::Produce(Message* msg)
{
    if (nullptr == msg)
    {
        ZS_LOG_ERROR(mq, "invalid message in mq producer, topic : %s", 
            _topic->name().c_str());
        return false;
    }

    if (nullptr == _producer)
    {
        ZS_LOG_ERROR(mq, "invalid producer in mq producer, topic : %s, key : %s, sn : %llu", 
            _topic->name().c_str(), msg->_key.c_str(), msg->_sn);
        return false;
    }

    // TODO: add headers
    // RdKafka::Headers *headers = RdKafka::Headers::create();
    // for (const auto& [key, value] : msg->_headers)
    // {
    //     headers->add(key, value);
    // }

    RdKafka::ErrorCode errCode = _producer->produce(_topic, RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY, (void*)msg->_buf.c_str(), msg->_buf.size(),
        &msg->_key, msg);
    if (RdKafka::ERR_NO_ERROR != errCode)
    {
        ZS_LOG_ERROR(mq, "producing message failed in mq producer, topic : %s, key : %s, sn : %llu, err code : %d", 
            _topic->name().c_str(), msg->_key.c_str(), msg->_sn, errCode);
        return false;
    }

    // polling is processed by pollers
    //_producer->poll(0);

    return true;
}

int32_t InternalProducer::Poll(int32_t timeoutMs)
{
    if (nullptr != _producer)
    {
        return _producer->poll(timeoutMs);
    }

    return 0;
}

InternalProducer::InternalProducer(Manager& manager)
    : _manager(manager)
{

}

InternalProducer::~InternalProducer()
{
    Finalize();
}
