
#include    "poller.h"

using namespace zs::common;
using namespace zs::mq;

bool Poller::Initialize(int32_t pollingTimeoutMs, int32_t pollingIntervalMs)
{
    _timeoutMs = pollingTimeoutMs;

    if (0 < pollingIntervalMs)
    {
        // _intervalMs is 10 by default
        _intervalMs = pollingIntervalMs;
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
