
#include    "mq/mq.h"

#include    "common/log.h"

#include    "rdkafkacpp.h"
#include    "manager.h"
#include    "producer.h"

using namespace zs::common;
using namespace zs::mq;

std::unordered_map<RdKafka::Event::Severity, LogLevel> Manager::EventCallbacker::ERROR_LEVEL = {
    {RdKafka::Event::EVENT_SEVERITY_EMERG,      LOGLEVEL_FATAL},
    {RdKafka::Event::EVENT_SEVERITY_ALERT,      LOGLEVEL_FATAL},
    {RdKafka::Event::EVENT_SEVERITY_CRITICAL,   LOGLEVEL_FATAL},
    {RdKafka::Event::EVENT_SEVERITY_ERROR,      LOGLEVEL_ERROR},
    {RdKafka::Event::EVENT_SEVERITY_WARNING,    LOGLEVEL_WARN},
    {RdKafka::Event::EVENT_SEVERITY_NOTICE,     LOGLEVEL_INFO},
    {RdKafka::Event::EVENT_SEVERITY_INFO,       LOGLEVEL_INFO},
    {RdKafka::Event::EVENT_SEVERITY_DEBUG,      LOGLEVEL_DEBUG}
};

bool Manager::Initialize(const ConfigList& configs, EventCallback ecb, ProducingCallback pcb)
{
    if (nullptr != _conf)
    {
        ZS_LOG_ERROR(mq, "internal producer already initialized");
        return false;
    }

    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

    std::string errMsg;

    // setting configs
    for (const auto& [key, value] : configs)
    {
        if (RdKafka::Conf::CONF_OK != conf->set(key, value, errMsg))
        {
            ZS_LOG_ERROR(mq, "setting config in mq manager failed, key : %s, value : %s, err : %s", 
                key.c_str(), value.c_str(), errMsg.c_str());
            delete conf;
            return false;
        }
    }

    // set event callback
    if (RdKafka::Conf::CONF_OK != conf->set("event_cb", &_eventCallbacker, errMsg))
    {
        ZS_LOG_ERROR(mq, "setting event callback config in mq manager failed, err : %s", 
            errMsg.c_str());
        delete conf;
        return false;
    }

    // set producing callback
    if (RdKafka::Conf::CONF_OK != conf->set("dr_cb", &_producingCallbacker, errMsg))
    {
        ZS_LOG_ERROR(mq, "setting producing callback config in mq manager failed, err : %s", 
            errMsg.c_str());
        delete conf;
        return false;
    }

    _conf = conf;
    _ecb = ecb;
    _pcb = pcb;

    ZS_LOG_INFO(mq, "mq manager initialized");

    return true;
}

void Manager::Finalize()
{
    for (const auto& [topic, prod] : _producers)
    {
        prod->Finalize();
    }
    _producers.clear();

    if (nullptr != _conf)
    {
        delete _conf;
        _conf = nullptr;
    }
}

ProducerSPtr Manager::CreateProducer(const std::string topic, const ConfigList& configs)
{
    if (nullptr == _conf)
    {
        ZS_LOG_ERROR(mq, "mq manager not initialized");
        return nullptr;
    }

    InternalProducerSPtr intProd = std::make_shared<InternalProducer>(*this);
    if (false == intProd->Initialize(topic, _conf, configs))
    {
        ZS_LOG_ERROR(mq, "initializing producer failed in mq manager, topic : %s",
            topic.c_str());
        return nullptr;
    }

    ProducerSPtr prod = std::make_shared<Producer>();
    prod->setInternalProducer(intProd);

    _producers[topic] = intProd;

    return prod;
}

Manager::Manager()
    : _eventCallbacker(*this), _producingCallbacker(*this)
{

}

void Manager::EventCallbacker::event_cb(RdKafka::Event &event)
{
    EventType type = EVENTTYPE_LOG;
    LogLevel level = LOGLEVEL_INFO;
    std::string msg;

    switch (event.type())
    {
    case RdKafka::Event::EVENT_ERROR:
        type = EVENTTYPE_ERROR;
        level = LOGLEVEL_ERROR;

        if (event.fatal())
        {
            level = LOGLEVEL_FATAL;
        }
        msg = RdKafka::err2str(event.err()) + ": " + event.str();
        ZS_LOG_ERROR(mq, "error event occurred in mq event callback, msg : %s",
            msg.c_str());
        break;

    case RdKafka::Event::EVENT_STATS:
        type = EVENTTYPE_STATS;
        msg = event.str();
        break;

    case RdKafka::Event::EVENT_LOG:
        level = ERROR_LEVEL[event.severity()];
        msg = event.fac() + ": " + event.str();
        break;

    default:
        msg = RdKafka::err2str(event.err()) + ": " + event.str();
        ZS_LOG_WARN(mq, "unknown event occurred in mq event callback, type : %d, msg : %s",
            event.type(), msg.c_str());
        return;
    }

    if (nullptr != _manager._ecb)
    {
        _manager._ecb(type, level, msg);
    }
}

Manager::EventCallbacker::EventCallbacker(Manager& manager)
    : _manager(manager)
{

}

Manager::ProducingCallbacker::ProducingCallbacker(Manager& manager)
    : _manager(manager)
{

}

void Manager::ProducingCallbacker::dr_cb(RdKafka::Message &message)
{
    MessageStatus status;
    Message* msg = static_cast<Message*>(message.msg_opaque());

    switch (message.status())
    {
    case RdKafka::Message::MSG_STATUS_NOT_PERSISTED:
        status = MESSAGESTATUS_NOT_PERSISTED;
        break;
    case RdKafka::Message::MSG_STATUS_POSSIBLY_PERSISTED:
        status = MESSAGESTATUS_POSSIBLY_PERSISTED;
        break;
    case RdKafka::Message::MSG_STATUS_PERSISTED:
        status = MESSAGESTATUS_PERSISTED;
        break;
    default:
        ZS_LOG_WARN(mq, "invalid message status in delivery callback, status : %d",
            message.status());
        return;
    }

    if (nullptr != _manager._pcb)
    {
        _manager._pcb(status, msg, message.errstr());
    }
}
