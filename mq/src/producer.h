
#ifndef __ZS_MQ_PRODUCER_H__
#define __ZS_MQ_PRODUCER_H__

#include    "common/types.h"
#include    "common/log.h"

#include    "mq/mq.h"
#include    "manager.h"
#include    "rdkafkacpp.h"

namespace zs
{
namespace mq
{
    using namespace zs::common;

    class InternalProducer final
    {
    public:
        bool Initialize(const std::string& topic, RdKafka::Conf* gConfig, const ConfigList& configs);
        void Finalize();

        bool Produce(Message* msg);

        InternalProducer(Manager& manager);
        ~InternalProducer();

    private:
        Manager&                        _manager;
        RdKafka::Conf*                  _conf = nullptr;
        RdKafka::Producer*              _producer = nullptr;
        RdKafka::Topic*                 _topic = nullptr;
    };
}
}

#endif // __ZS_MQ_PRODUCER_H__