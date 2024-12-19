
#include    "msg_counter.h"

#include    "common/log.h"

#include    <chrono>

using namespace zs::common;

void MsgCounter::IncreaseCount()
{
    _cnt++;
}

void MsgCounter::threadMain()
{
    while (THREAD_STATUS_RUNNING == getStatus())
    {
        auto now = std::chrono::system_clock::now();
        auto epochSec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        if (_tick < epochSec)
        {
            _tick = epochSec;
            uint64_t cnt = _cnt.exchange(0);
            ZS_LOG_INFO(mq_test_producer, "current received message count per second : %llu",
                cnt);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
