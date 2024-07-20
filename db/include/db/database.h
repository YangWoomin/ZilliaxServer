
#ifndef __ZS_DATABASE_DATABASE_H__
#define __ZS_DATABASE_DATABASE_H__

#ifdef _MSVC_
#include    <windows.h>
#endif

#include    <sql.h>
#include    <sqlext.h>
#include	<sqltypes.h>

namespace zs
{
namespace db
{
    class Database
    {
    private:
        static void HandleSQLError(SQLHANDLE handle, SQLSMALLINT handleType);
    friend class Worker;
    };
}
}

#endif // __ZS_DATABASE_DATABASE_H__
