
#ifndef __ZS_MQ_MQ_H__
#define __ZS_MQ_MQ_H__


#if defined(_WIN64_)

#ifdef ZS_MQ_EXPORTS
#define __ZS_MQ_API __declspec(dllexport)
#else // ZS_MQ_EXPORTS
#define __ZS_MQ_API __declspec(dllimport)
#endif

#elif defined(_POSIX_) 

#ifdef ZS_MQ_EXPORTS
#define __ZS_MQ_API __attribute__((visibility ("default")))
#else // ZS_MQ_EXPORTS
#define __ZS_MQ_API 
#endif

#endif // _WIN64_

#include    "common/types.h"
#include    "common/log.h"

#include    <functional>
#include    <memory>
#include    <unordered_map>
#include    <vector>

namespace zs
{
namespace mq
{
    using namespace zs::common;

    enum EventType
    {
        EVENTTYPE_ERROR,
        EVENTTYPE_STATS,
        EVENTTYPE_LOG,
        EVENTTYPE_THROTTLE,
    };

    using EventCallback = std::function<void(EventType, LogLevel, const std::string&)>;

    enum MessageStatus
    {
        MESSAGESTATUS_NOT_PERSISTED,
        MESSAGESTATUS_POSSIBLY_PERSISTED,
        MESSAGESTATUS_PERSISTED,
    };

    struct Message
    {
        std::string                                         _key;
        std::string                                         _buf;
        //std::unordered_map<std::string, std::string>        _headers; // TODO
        uint64_t                                            _sn = 0;
    };

    using ProducingCallback = std::function<void(MessageStatus, Message*, const std::string&)>;

    class InternalProducer;
    using InternalProducerSPtr = std::shared_ptr<InternalProducer>;
    using InternalProducerWPtr = std::weak_ptr<InternalProducer>;

    class Producer final
    {
    public:
        // thread safe function
        __ZS_MQ_API bool Produce(Message* msg);

    private:
        void setInternalProducer(InternalProducerWPtr intProd)
        {
            _intProd = intProd;
        }

        InternalProducerWPtr    _intProd;

    friend class Manager;
    };

    using ProducerSPtr = std::shared_ptr<Producer>;

    using ConfigList = std::vector<std::pair<std::string, std::string>>;
    
    class __ZS_MQ_API MQ final
    {
    public:
        static bool Initialize(Logger::Messenger msgr, const ConfigList& configs, EventCallback ecb, ProducingCallback pcb);
        static void Finalize();

        static ProducerSPtr CreateProducer(const std::string topic, const ConfigList& configs);

    private:

        MQ() = delete;
        ~MQ() = delete;
        MQ(const MQ&) = delete;
        MQ& operator=(const MQ&) = delete;
    };
}
}

#endif // __ZS_MQ_MQ_H__
