
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

    if (!SQL_SUCCEEDED(SQLPrepare(_hStmt, (SQLCHAR*)_query.c_str(), SQL_NTS)))
    {
        HandleSQLError(_hStmt, SQL_HANDLE_STMT, "SQLPrepare failed, cid : %llu, query : %s", _cid, _query.c_str());
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
        ZS_LOG_ERROR(db, "bindParams failed, cid : %llu, query : %s", _cid, _query.c_str());
        return false;
    }

    _params.push_back(params);

    return true;
}

bool Operation::Execute(ResultSetSPtr rs)
{
    SQLRETURN retcode = SQLExecute(_hStmt);
    //if (!SQL_SUCCEEDED(SQLExecute(_hStmt)))
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
        HandleSQLError(_hStmt, SQL_HANDLE_STMT, "SQLExecute failed, cid : %llu, query : %s", _cid, _query.c_str());
        return false;
    }

    if (nullptr != rs)
    {
        // bind columns
        if (false == rs->bindColumns(_hStmt, _cid))
        {
            // HandleSQLError already called
            ZS_LOG_ERROR(db, "bindColumns failed, cid : %llu, query : %s", _cid, _query.c_str());
            return false;
        }

        // fetch
        rs->fetch(_hStmt);
    }

    clearStatement();

    return true;
}

void Operation::Clear()
{
    _query = "";

    _params.clear();

    clearStatement();
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
    SQLRETURN resCode;

    resCode = SQLFreeStmt(_hStmt, SQL_CLOSE);
    if (!SQL_SUCCEEDED(resCode))
    {
        HandleSQLError(_hStmt, SQL_HANDLE_STMT, "SQLFreeStmt for SQL_CLOSE failed");
        return;
    }

    resCode = SQLFreeStmt(_hStmt, SQL_UNBIND);
    if (!SQL_SUCCEEDED(resCode))
    {
        HandleSQLError(_hStmt, SQL_HANDLE_STMT, "SQLFreeStmt for SQL_UNBIND failed");
        return;
    }

    resCode = SQLFreeStmt(_hStmt, SQL_RESET_PARAMS);
    if (!SQL_SUCCEEDED(resCode))
    {
        HandleSQLError(_hStmt, SQL_HANDLE_STMT, "SQLFreeStmt for SQL_RESET_PARAMS failed");
        return;
    }
}

void Operation::complete()
{
    if (!SQL_SUCCEEDED(SQLSetConnectAttr(_hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0)))
    {
        HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLSetConnectAttr failed in EndTransaction, cid : %llu", _cid);
    }
}

TransactionalOperation::TransactionalOperation(contextID cid)
    : Operation(cid)
{

}

bool TransactionalOperation::BeginTransaction()
{
    if (!SQL_SUCCEEDED(SQLSetConnectAttr(_hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0)))
    {
        HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLSetConnectAttr failed in BeginTransaction, cid : %llu", _cid);
        return false;
    }

    return true;
}

bool TransactionalOperation::RollbackTransaction()
{
    if (!SQL_SUCCEEDED(SQLEndTran(SQL_HANDLE_DBC, _hDbc, SQL_ROLLBACK)))
    {
        HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLEndTran failed in RollbackTransaction, cid : %llu", _cid);
        return false;
    }

    return true;
}

bool TransactionalOperation::CommitTransaction()
{
    if (!SQL_SUCCEEDED(SQLEndTran(SQL_HANDLE_DBC, _hDbc, SQL_COMMIT)))
    {
        HandleSQLError(_hDbc, SQL_HANDLE_DBC, "SQLEndTran failed in CommitTransaction, cid : %llu", _cid);
        return false;
    }

    return true;
}
