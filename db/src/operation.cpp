
#include    "db/operation.h"
#include    "internal_common.h"

using namespace zs::common;
using namespace zs::db;

Operation::Operation(contextID cid)
    : _cid(cid)
{

}

contextID Operation::GetContextID() const
{
    return _cid;
}

bool Operation::Prepare(const char* query)
{
    if (nullptr == query)
    {
        return false;
    }

    _query = query;

    SQLRETURN ret = SQLPrepare(_hStmt, (SQLCHAR*)_query.c_str(), SQL_NTS);

    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(_hStmt, SQL_HANDLE_STMT, "SQLPrepare failed, ret : %d, query : %s, cid : %llu", ret, _query.c_str(), _cid);
        return false;
    }

    return true;
}

bool Operation::BindParams(ParamsSPtr params)
{
    if (nullptr == params)
    {
        return false;
    }

    if (false == params->bindParams(_hStmt, _cid))
    {
        // HandleSQLError already called
        ZS_LOG_ERROR(db, "bindParams failed, query : %s, cid : %llu", _query.c_str(), _cid);
        return false;
    }

    _params.push_back(params);

    return true;
}

bool Operation::Execute(ResultSetSPtr rs)
{
    SQLRETURN ret = SQLExecute(_hStmt);

    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(_hStmt, SQL_HANDLE_STMT, "SQLExecute failed, ret : %d, query : %s, cid : %llu", ret, _query.c_str(), _cid);
        return false;
    }

    if (nullptr != rs)
    {
        // bind columns
        if (false == rs->bindColumns(_hStmt, _cid))
        {
            // HandleSQLError already called
            ZS_LOG_ERROR(db, "bindColumns failed, query : %s, cid : %llu", _query.c_str(), _cid);
            return false;
        }

        _rs = rs;

        // fetch
        rs->fetch(_hStmt);
    }

    clearStatement();

    return true;
}

void Operation::execute(SQLHDBC hDbc, SQLHSTMT hStmt)
{
    _hDbc = hDbc;
    _hStmt = hStmt;

    // process user logic
    process();

    // complete statement
    complete();
}

void Operation::clearStatement()
{
    SQLRETURN ret;

    ret = SQLFreeStmt(_hStmt, SQL_CLOSE);
    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(_hStmt, SQL_HANDLE_STMT, "SQLFreeStmt for SQL_CLOSE failed, ret : %d, cid : %llu", ret, _cid);
        return;
    }

    ret = SQLFreeStmt(_hStmt, SQL_UNBIND);
    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(_hStmt, SQL_HANDLE_STMT, "SQLFreeStmt for SQL_UNBIND failed, ret : %d, cid : %llu", ret, _cid);
        return;
    }

    ret = SQLFreeStmt(_hStmt, SQL_RESET_PARAMS);
    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(_hStmt, SQL_HANDLE_STMT, "SQLFreeStmt for SQL_RESET_PARAMS failed, ret : %d, cid : %llu", ret, _cid);
        return;
    }
}

void Operation::complete()
{
    SQLRETURN ret = SQLSetConnectAttr(_hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLSetConnectAttr failed in EndTransaction, ret : %d, cid : %llu", ret, _cid);
    }
}

TransactionalOperation::TransactionalOperation(contextID cid)
    : Operation(cid)
{

}

bool TransactionalOperation::BeginTransaction()
{
    SQLRETURN ret = SQLSetConnectAttr(_hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLSetConnectAttr failed in BeginTransaction, ret : %d, cid : %llu", ret, _cid);
        return false;
    }

    return true;
}

bool TransactionalOperation::RollbackTransaction()
{
    SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, _hDbc, SQL_ROLLBACK);
    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLEndTran failed in RollbackTransaction, ret : %d, cid : %llu", ret, _cid);
        return false;
    }

    return true;
}

bool TransactionalOperation::CommitTransaction()
{
    SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, _hDbc, SQL_COMMIT);
    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLEndTran failed in CommitTransaction, ret : %d, cid : %llu", ret, _cid);
        return false;
    }

    return true;
}
