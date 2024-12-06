#pragma once

#include    "common/types.h"
#include    "common/log.h"

#include    "mq/mq.h"

#include    <functional>

using namespace zs::common;
using namespace zs::mq;

class MQProducer final
{
public:
    bool Initialize(Logger::Messenger msgr, const std::string servers, const std::string debug, int32_t pollerCount, int32_t intervalMs);
    void Finalize();

    bool CreateProducer(std::string topic);
    bool Produce(const char* key, const char* payload, std::size_t len, uint64_t sn);

    ~MQProducer();

private:
    ProducerSPtr    _producer;
};
