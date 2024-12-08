
#ifndef __ZS_CACHE_WORKER_H__
#define __ZS_CACHE_WORKER_H__

#include    <memory>
#include    <string>
#include    <queue>
#include    <mutex>
#include    <condition_variable>

#include    "common/types.h"
#include    "common/thread.h"

#include    "cache/cache.h"
#include    "common.h"

#include    <sw/redis++/redis++.h>
#include    <sw/redis++/async_redis++.h>

namespace zs
{
namespace cache
{
    using namespace zs::common;
    using namespace sw::redis;

    struct Operation
    {
        Script                          _script;
        ContextID                       _cid = 0;
        Keys                            _keys;
        Args                            _args;

        virtual void Callback() = 0;
    };

    struct Operation1 : public Operation
    {
        Future<Optional<ResultSet1>>     _fut;
        AsyncSet1Callback                _cb = nullptr;

        virtual void Callback() override;
    };
    using Operation1SPtr = std::shared_ptr<Operation1>;

    struct Operation2 : public Operation
    {
        Future<Optional<SimpleResult>>   _fut;
        AsyncSet2Callback                _cb = nullptr;

        virtual void Callback() override;
    };
    using Operation2SPtr = std::shared_ptr<Operation2>;

    using OperationSPtr = std::shared_ptr<Operation>;

    class Worker final : public Thread<Worker>
    {
    public:
        bool Post(OperationSPtr op);

        virtual void Stop() override final;

        Worker() = default;
        ~Worker() = default;

    private:
        std::queue<OperationSPtr>   _ops;
        std::mutex                  _lock;
        std::condition_variable     _signal;

        void threadMain();

    friend class Thread<Worker>;
    };
}
}

#endif // __ZS_CACHE_WORKER_H__
