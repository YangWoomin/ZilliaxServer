
#include    <sql.h>
#include    <sqlext.h>

#include    "common/log.h"
#include    "db/database.h"

void HandleSQLError(SQLHANDLE handle, SQLSMALLINT handleType)
{
    SQLCHAR sqlState[SQL_SQLSTATE_SIZE + 1] = { 0, };
    SQLINTEGER nativeError = 0;
    SQLCHAR messageText[SQL_MAX_MESSAGE_LENGTH] = { 0, };
    SQLSMALLINT textLength = 0;
    SQLSMALLINT i = 1;
    SQLRETURN ret;

    while (SQL_NO_DATA != (ret = SQLGetDiagRec(handleType, handle, i++, sqlState, &nativeError, messageText, sizeof(messageText), &textLength))) 
    {
        if (SQL_SUCCEEDED(ret)) 
        {
            ZS_LOG_ERROR(db, "sql state: %s, native error: %d, message: %s", sqlState, nativeError, messageText);
        }
    }
}


