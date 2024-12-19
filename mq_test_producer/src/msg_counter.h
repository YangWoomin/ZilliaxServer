
#include    "common/types.h"
#include    "common/thread.h"

#include    <atomic>

using namespace zs::common;

class MsgCounter final : public Thread<MsgCounter>
{
public:
    void IncreaseCount();

    MsgCounter() = default;
    ~MsgCounter() = default;

private:
    std::atomic<uint64_t>       _cnt {0};

    int64_t                     _tick = 0;

    void threadMain();

friend class Thread<MsgCounter>;
};
