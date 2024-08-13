
#ifndef __ZS_DB_DATABASE_H__
#define __ZS_DB_DATABASE_H__

#if  defined(_WIN64_)
#include    <windows.h>
#endif

#include    <sql.h>
#include    <sqlext.h>
#include	<sqltypes.h>

#include    <vector>

#include    "db/common.h"

namespace zs
{
namespace db
{
    using namespace zs::common;

    class Worker;

    class __ZS_DB_API Database final
    {
    public:
        Database() = default;
        ~Database() = default;

        bool Initialize(const Config& config, Logger::Messenger msgr);
        void Finalize();

        bool Start();
        void Stop();

        bool Post(std::size_t workerNum, OperationSPtr op);

    private:
        bool                    _init = false;
        std::vector<WorkerSPtr> _workers;
        Logger::Messenger       _msgr = nullptr;
    };
}
}

#endif // __ZS_DB_DATABASE_H__
