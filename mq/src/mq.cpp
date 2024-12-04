
#include    "mq/mq.h"

#include    "common/log.h"

#include    "rdkafkacpp.h"

using namespace zs::common;
using namespace zs::mq;

void MQ::Initialize()
{
    std::string errStr;
    RdKafka::Producer* proc = RdKafka::Producer::create(nullptr, errStr);
    if (nullptr != proc)
    {
        proc->flush(100);
    }
}
