
#include    "mq/mq.h"

#include    "common/log.h"

#include    "manager.h"
#include    "producer.h"

using namespace zs::common;
using namespace zs::mq;

static Manager* manager = nullptr;

bool Producer::Produce(Message* msg)
{
    InternalProducerSPtr intProd = _intProd.lock();
    if (nullptr != intProd)
    {
        return intProd->Produce(msg);
    }

    ZS_LOG_ERROR(mq, "invalid producer, key : %s, sn : %llu",
        msg->_key.c_str(), msg->_sn);

    return false;
}

bool MQ::Initialize(Logger::Messenger msgr, const ConfigList& configs, EventCallback ecb, ProducingCallback pcb, int32_t pollerCount, int32_t pollingTimeoutMs, int32_t pollingIntervalMs)
{
    Logger::Messenger& messenger = Logger::GetMessenger();
    messenger = msgr;

    if (nullptr != manager)
    {
        ZS_LOG_ERROR(mq, "mq module already initialized");
        return false;
    }

    manager = new Manager();

    if (false == manager->Initialize(configs, ecb, pcb, pollerCount, pollingTimeoutMs, pollingIntervalMs))
    {
        ZS_LOG_ERROR(mq, "initializing mq manager failed");
        return false;
    }

    ZS_LOG_INFO(mq, "mq module initialized");

    return true;
}

void MQ::Finalize()
{
    if (nullptr != manager)
    {
        manager->Finalize();
        delete manager;
        manager = nullptr;
    }
}

ProducerSPtr MQ::CreateProducer(const std::string topic, const ConfigList& configs)
{
    if (nullptr == manager)
    {
        ZS_LOG_ERROR(mq, "mq module not initialized");
        return nullptr;
    }

    return manager->CreateProducer(topic, configs);
}
