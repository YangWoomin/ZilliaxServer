
#ifndef __ZS_DATABASE_WORKER_H__
#define __ZS_DATABASE_WORKER_H__

#ifdef _MSVC_
#include    <windows.h>
#endif

#include    <sql.h>
#include    <sqlext.h>
#include	<sqltypes.h>

#include    <string>

#include    "common/types.h"
#include    "common/thread.h"

namespace zs
{
namespace db
{
    struct Config
    {
        std::string     _dsn;
        uint16_t        _connTimeout;
        uint16_t        _stmtTimeout;
        uint32_t        _connTryInterval;
        uint32_t        _connTryCount;
    };

    class Worker final : zs::common::Thread<Worker>
    {
    public:
        Worker() = default;
        virtual ~Worker() = default;

        bool Initialize(const Config& config);
        void Finalize();

    private:
        bool connect(const Config& config);
        void disconnect();
        bool isConnected();

        // bool beginTransaction();
        // bool endTransaction();
        // bool commitTransaction();
        // bool rollbackTransaction();

        void threadMain();

        bool        _init = false;
        Config      _config;
        SQLHENV     _hEnv = nullptr;
        SQLHDBC     _hDbc = nullptr;
        SQLHSTMT    _hStmt = nullptr;

    friend class zs::common::Thread<Worker>;
    //friend class zs::db::Operation;
    };
}
}

#endif // __ZS_DATABASE_WORKER_H__
