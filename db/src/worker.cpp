
#include    "worker.h"
#include    "common/log.h"
#include    "db/database.h"
#include    "internal_common.h"

using namespace zs::common;
using namespace zs::db;

bool Worker::Initialize(const Config& config)
{
    if (true == _init)
    {
        return false;
    }

    SQLRETURN resCode;

    resCode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_hEnv);
    if (!SQL_SUCCEEDED(resCode))
    {
        HandleSQLError(_hEnv, SQL_HANDLE_ENV, "SQLAllocHandle for hEnv failed");
        Finalize();
        return false;
    }

    resCode = SQLSetEnvAttr(_hEnv, SQL_ATTR_ODBC_VERSION, (void*) SQL_OV_ODBC3, 0);
    if (!SQL_SUCCEEDED(resCode))
    {
        HandleSQLError(_hEnv, SQL_HANDLE_ENV, "SQLSetEnvAttr for hEnv failed");
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

void Worker::Stop()
{
    Thread<Worker>::Stop();

    _signal.notify_one();

    Join();
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

bool Worker::Post(OperationSPtr op)
{
    if (THREAD_STATUS_RUNNING != getStatus())
    {
        return false;
    }

    bool signal = false;
    {
        _lock.lock();
        signal = _ops.empty();
        _ops.push(op);
        _lock.unlock();
    }
    
    if (true == signal)
    {
        _signal.notify_one();
    }

    return true;
}

bool Worker::connect(const Config& config)
{
    SQLRETURN resCode;

    resCode = SQLAllocHandle(SQL_HANDLE_DBC, _hEnv, &_hDbc);
    if (!SQL_SUCCEEDED(resCode))
    {
        HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLAllocHandle for hDbc failed");
        return false;
    }

    // set connection timeout
    resCode = SQLSetConnectAttr(_hDbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)config._connTimeout, 0);
    if (!SQL_SUCCEEDED(resCode))
    {
        HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLSetConnectAttr for conn timeout failed");
        return false;
    }

    // enable auto commit
    resCode = SQLSetConnectAttr(_hDbc, SQL_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
    if (!SQL_SUCCEEDED(resCode))
    {
        HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLSetConnectAttr for auto commit on failed");
        return false;
    }

    // connect
    resCode = SQLDriverConnect(_hDbc, NULL, (SQLCHAR*)config._dsn.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (!SQL_SUCCEEDED(resCode))
    {
        HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLDriverConnect failed, dsn : %s", config._dsn.c_str());
        return false;
    }

    resCode = SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &_hStmt);
    if (!SQL_SUCCEEDED(resCode))
    {
        HandleSQLError(_hStmt, SQL_HANDLE_STMT, "SQLAllocHandle for hStmt failed");
        return false;
    }

    resCode = SQLSetStmtAttr(_hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)config._stmtTimeout, 0);
    if (!SQL_SUCCEEDED(resCode))
    {
        HandleSQLError(_hStmt, SQL_HANDLE_STMT, "SQLSetStmtAttr for query timeout failed");
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
            HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLDisconnect failed");
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
        HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLGetConnectAttr for check failed");
        return false;
    }
    return connectionDead == SQL_CD_FALSE;
}

bool Worker::reconnect()
{
    for (uint32_t i = 0; i < _config._connTryCount; ++i)
    {
        if (true == connect(_config))
        {
            return true;
        }

        std::this_thread::sleep_for(std::chrono::seconds(_config._connTryInterval));
    }

    return false;
}

void Worker::threadMain()
{
    std::unique_lock<std::mutex> ulock { _lock, std::defer_lock };
    while (THREAD_STATUS_RUNNING == getStatus())
    {
        ulock.lock();

        if (true == _ops.empty())
        {
            _signal.wait(ulock);
        }

        if (false == _ops.empty())
        {
            OperationSPtr op = _ops.front();
            _ops.pop();
            ulock.unlock();
            runOp(op);
        }
        else
        {
            ulock.unlock();
        }
    }

    ulock.lock();
    while (false == _ops.empty())
    {
        OperationSPtr op = _ops.front();
        _ops.pop();
        runOp(op);
    }
    ulock.unlock();
}

void Worker::runOp(OperationSPtr op)
{
    if (false == isConnected() && false == reconnect())
    {
        ZS_LOG_ERROR(db, "operation failed by database disconnected, cid : %llu", op->GetContextID());
        op->failed(OPERATION_FAILURE_REASON_DISCONNECTED);
        return;
    }

    op->execute(_hDbc, _hStmt);
}
