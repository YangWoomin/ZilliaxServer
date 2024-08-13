
#ifndef __ZS_DB_INTERNAL_COMMON_H__
#define __ZS_DB_INTERNAL_COMMON_H__

#if  defined(_WIN64_)
#include    <windows.h>
#endif

#include    <sql.h>
#include    <sqlext.h>
#include	<sqltypes.h>

#include    <string>

#include    "common/types.h"
#include    "common/log.h"

namespace zs
{
namespace db
{
    using namespace zs::common;

    template <typename... Args>
    void HandleSQLError(SQLHANDLE handle, SQLSMALLINT handleType, const char* fmt, Args... args)
    {
        SQLCHAR sqlState[SQL_SQLSTATE_SIZE + 1] = { 0, };
        SQLINTEGER nativeError = 0;
        SQLCHAR messageText[SQL_MAX_MESSAGE_LENGTH] = { 0, };
        SQLSMALLINT textLength = 0;
        SQLSMALLINT i = 1;
        SQLRETURN ret;

        std::string fmt_;
        fmt_ += "sql state: %s, native error: %d, native message: %s, db message : [";
        fmt_ += fmt;
        fmt_ += "]";

        bool outputted = false;
        while (SQL_NO_DATA != (ret = SQLGetDiagRec(handleType, handle, i++, sqlState, &nativeError, messageText, sizeof(messageText), &textLength))) 
        {
            if (SQL_SUCCEEDED(ret)) 
            {
                ZS_LOG_ERROR(db, fmt_.c_str(), sqlState, nativeError, messageText, args...);
                outputted = true;
            }
        }

        if (false == outputted)
        {
            ZS_LOG_ERROR(db, fmt, args...);
        }
    }
}
}


#endif // __ZS_DB_INTERNAL_COMMON_H__
