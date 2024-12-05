
#ifndef __ZS_MQ_MANAGER_H__
#define __ZS_MQ_MANAGER_H__

#include    "common/types.h"
#include    "common/log.h"

#include    "mq/mq.h"
#include    "rdkafkacpp.h"

#include    <unordered_map>

namespace zs
{
namespace mq
{
    using namespace zs::common;

    class Manager final
    {
    public:
        bool Initialize(const ConfigList& configs, EventCallback ecb, ProducingCallback pcb);
        void Finalize();

        ProducerSPtr CreateProducer(const std::string topic, const ConfigList& configs);

        Manager();

    private:
        RdKafka::Conf*                                          _conf = nullptr;
        EventCallback                                           _ecb = nullptr;
        ProducingCallback                                       _pcb = nullptr;
        std::unordered_map<std::string, InternalProducerSPtr>   _producers;

        struct EventCallbacker : public RdKafka::EventCb
        {
            EventCallbacker(Manager&);
            Manager& _manager;
            static std::unordered_map<RdKafka::Event::Severity, LogLevel> ERROR_LEVEL;

            void event_cb(RdKafka::Event &event);
        };
        EventCallbacker _eventCallbacker;

        struct ProducingCallbacker : public RdKafka::DeliveryReportCb
        {
            ProducingCallbacker(Manager&);
            Manager& _manager;

            void dr_cb(RdKafka::Message &message);
        };
        ProducingCallbacker _producingCallbacker;
    
    friend struct EventCallbacker;
    friend struct ProducingCallbacker;
    };
}
}

#endif // __ZS_MQ_MANAGER_H__
