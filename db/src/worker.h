
#ifndef __ZS_DATABASE_WORKER_H__
#define __ZS_DATABASE_WORKER_H__

#if  defined(_MSVC_)
#include    <windows.h>
#endif

#include    <sql.h>
#include    <sqlext.h>
#include	<sqltypes.h>

#include    <memory>
#include    <string>
#include    <queue>
#include    <mutex>
#include    <condition_variable>

#include    "common/types.h"
#include    "common/thread.h"

#include    "db/common.h"
#include    "db/operation.h"

namespace zs
{
namespace db
{
    using namespace zs::common;

    class Worker final : public Thread<Worker>
    {
    public:
        Worker() = default;
        virtual ~Worker() = default;

        bool Initialize(const Config& config);
        void Finalize();

        virtual void Stop() override;

        bool Post(OperationSPtr op);

    private:
        bool connect(const Config& config);
        void disconnect();
        bool isConnected();
        bool reconnect();

        bool                    _init = false;
        Config                  _config;
        SQLHENV                 _hEnv = nullptr;
        SQLHDBC                 _hDbc = nullptr;
        SQLHSTMT                _hStmt = nullptr;

    private:
        std::queue<OperationSPtr>   _ops;
        std::mutex                  _lock;
        std::condition_variable     _signal;

        void threadMain();
        void runOp(OperationSPtr op);

    friend class Thread<Worker>;
    };
}
}

#endif // __ZS_DATABASE_WORKER_H__
