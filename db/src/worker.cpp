
#include    "worker.h"
#include    "common/log.h"
#include    "db/database.h"

using namespace zs::db;

bool Worker::Initialize(const Config& config)
{
    if (true == _init)
    {
        return false;
    }

    SQLRETURN resCode = SQL_SUCCESS;

    resCode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_hEnv);
    if (!SQL_SUCCEEDED(resCode))
    {
        Database::HandleSQLError(_hEnv, SQL_HANDLE_ENV);
        Finalize();
        return false;
    }

    resCode = SQLSetEnvAttr(_hEnv, SQL_ATTR_ODBC_VERSION, (void*) SQL_OV_ODBC3, 0);
    if (!SQL_SUCCEEDED(resCode))
    {
        Database::HandleSQLError(_hEnv, SQL_HANDLE_ENV);
        Finalize();
        return false;
    }

    if (false == connect(config))
    {
        Finalize();
        return false;
    }

    _init = true;
    _config = config;

    ZS_LOG_INFO(db, "database initialization succeeded");

    return true;
}

void Worker::Finalize()
{
    Stop();

    if (nullptr != _hStmt)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, _hStmt);
        _hStmt = nullptr;
    }

    if (nullptr != _hDbc)
    {
        disconnect();
        SQLFreeHandle(SQL_HANDLE_DBC, _hDbc);
        _hDbc = nullptr;
    }

    if (nullptr != _hEnv)
    {
        SQLFreeHandle(SQL_HANDLE_ENV, _hEnv);
        _hEnv = nullptr;
    }

    _init = false;
}

bool Worker::connect(const Config& config)
{
    SQLRETURN resCode = SQL_SUCCESS;

    resCode = SQLAllocHandle(SQL_HANDLE_DBC, _hEnv, &_hDbc);
    if (!SQL_SUCCEEDED(resCode))
    {
        Database::HandleSQLError(_hDbc, SQL_HANDLE_DBC);
        return false;
    }

    // set connection timeout
    resCode = SQLSetConnectAttr(_hDbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)config._connTimeout, 0);
    if (!SQL_SUCCEEDED(resCode))
    {
        Database::HandleSQLError(_hDbc, SQL_HANDLE_DBC);
        return false;
    }

    // enable auto commit
    resCode = SQLSetConnectAttr(_hDbc, SQL_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
    if (!SQL_SUCCEEDED(resCode))
    {
        Database::HandleSQLError(_hDbc, SQL_HANDLE_DBC);
        return false;
    }

    // connect
    resCode = SQLDriverConnect(_hDbc, NULL, (SQLCHAR*)config._dsn.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (!SQL_SUCCEEDED(resCode))
    {
        Database::HandleSQLError(_hDbc, SQL_HANDLE_DBC);
        return false;
    }

    resCode = SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &_hStmt);
    if (!SQL_SUCCEEDED(resCode))
    {
        Database::HandleSQLError(_hStmt, SQL_HANDLE_STMT);
        return false;
    }

    resCode = SQLSetStmtAttr(_hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)config._stmtTimeout, 0);
    if (!SQL_SUCCEEDED(resCode))
    {
        Database::HandleSQLError(_hStmt, SQL_HANDLE_STMT);
        return false;
    }

    ZS_LOG_INFO(db, "odbc connected");

    return true;
}

void Worker::disconnect()
{
    if (true == isConnected())
    {
        SQLRETURN resCode = SQLDisconnect(_hDbc);
        if (!SQL_SUCCEEDED(resCode))
        {
            Database::HandleSQLError(_hDbc, SQL_HANDLE_DBC);
            return;
        }
    }
}

bool Worker::isConnected()
{
    SQLINTEGER connectionDead = SQL_CD_FALSE;
    SQLRETURN resCode = SQLGetConnectAttr(_hDbc, SQL_ATTR_CONNECTION_DEAD, &connectionDead, SQL_IS_INTEGER, NULL);
    if (!SQL_SUCCEEDED(resCode))
    {
        return false;
    }
    return connectionDead == SQL_CD_FALSE;
}
