
#include    "poller.h"

using namespace zs::common;
using namespace zs::mq;

bool Poller::Initialize(int32_t timeoutMs, int32_t intervalMs)
{
    _timeoutMs = timeoutMs;

    if (0 < intervalMs)
    {
        // _intervalMs is 10 by default
        _intervalMs = intervalMs;
    }
    
    return true;
}

void Poller::Finalize()
{
    Stop();
    Join();
}

void Poller::AddPollingBox(PollingBoxWPtr box)
{
    std::lock_guard<std::recursive_mutex> lock(_mtx);
    _boxes.push_back(box);
}

void Poller::threadMain()
{
    while (THREAD_STATUS_RUNNING == getStatus())
    {
        _mtx.lock();

        while (true)
        {
            int count = 0;
            const std::size_t size = _boxes.size();

            for (std::size_t i = 0; i < size; ++i)
            {
                PollingBoxSPtr box = _boxes[i].lock();
                if (nullptr != box)
                {
                    count += box->Poll(_timeoutMs);
                }
            }

            if (0 == count)
            {
                break;
            }
        }
        
        _mtx.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(_intervalMs));
    }
}
