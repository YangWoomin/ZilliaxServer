
#include    "db/result_set.h"
#include    "internal_common.h"

using namespace zs::common;
using namespace zs::db;

template <typename T>
bool bindCol(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, T& column, SQLLenSPtr& len, contextID cid)
{
    SQLRETURN ret = SQLBindCol(
        hStmt, idx + 1, cTypeHint,
        &column, sizeof(column), len.get());
    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(hStmt, SQL_HANDLE_STMT, "bindCol failed, ret : %d, idx : %lu, c type : %d, sql type : %d, len : %lld, cid : %llu", ret, idx + 1, cTypeHint, sqlTypeHint, *len, cid);
        return false;
    }

    return true;
}

template<>
bool bindCol<std::string>(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, std::string& column, SQLLenSPtr& len, contextID cid)
{
    if (0 == column.size())
    {
        column.resize(DEFAULT_BUFFER_COLUMN_LEN, NULL);
    }

    if (SQL_VARCHAR == sqlTypeHint)
    {
        *len = SQL_NTS;
    }
    else // if (SQL_BINARY == sqlTypeHint)
    {
        *len = column.size();
    }

    SQLRETURN ret = SQLBindCol(
        hStmt, idx + 1, cTypeHint,
        column.data(), column.size(), len.get());
    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(hStmt, SQL_HANDLE_STMT, "bindCol failed for string, ret : %d, idx : %lu, c type : %d, sql type : %d, len : %lld, cid : %llu", ret, idx + 1, cTypeHint, sqlTypeHint, *len, cid);
        return false;
    }

    return true;
}

bool ColBinder::BindCol(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, int8_t& column, SQLLenSPtr& len, contextID cid)
{
    return bindCol(hStmt, idx, cTypeHint, sqlTypeHint, column, len, cid);
}

bool ColBinder::BindCol(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, uint8_t& column, SQLLenSPtr& len, contextID cid)
{
    return bindCol(hStmt, idx, cTypeHint, sqlTypeHint, column, len, cid);
}

bool ColBinder::BindCol(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, int16_t& column, SQLLenSPtr& len, contextID cid)
{
    return bindCol(hStmt, idx, cTypeHint, sqlTypeHint, column, len, cid);
}

bool ColBinder::BindCol(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, uint16_t& column, SQLLenSPtr& len, contextID cid)
{
    return bindCol(hStmt, idx, cTypeHint, sqlTypeHint, column, len, cid);
}

bool ColBinder::BindCol(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, int32_t& column, SQLLenSPtr& len, contextID cid)
{
    return bindCol(hStmt, idx, cTypeHint, sqlTypeHint, column, len, cid);
}

bool ColBinder::BindCol(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, uint32_t& column, SQLLenSPtr& len, contextID cid)
{
    return bindCol(hStmt, idx, cTypeHint, sqlTypeHint, column, len, cid);
}

bool ColBinder::BindCol(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, int64_t& column, SQLLenSPtr& len, contextID cid)
{
    return bindCol(hStmt, idx, cTypeHint, sqlTypeHint, column, len, cid);
}

bool ColBinder::BindCol(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, uint64_t& column, SQLLenSPtr& len, contextID cid)
{
    return bindCol(hStmt, idx, cTypeHint, sqlTypeHint, column, len, cid);
}

bool ColBinder::BindCol(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, float32_t& column, SQLLenSPtr& len, contextID cid)
{
    return bindCol(hStmt, idx, cTypeHint, sqlTypeHint, column, len, cid);
}

bool ColBinder::BindCol(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, float64_t& column, SQLLenSPtr& len, contextID cid)
{
    return bindCol(hStmt, idx, cTypeHint, sqlTypeHint, column, len, cid);
}

bool ColBinder::BindCol(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, std::string& column, SQLLenSPtr& len, contextID cid)
{
    return bindCol(hStmt, idx, cTypeHint, sqlTypeHint, column, len, cid);
}

bool ColBinder::BindCol(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, bytes_t& column, SQLLenSPtr& len, contextID cid)
{
    return bindCol(hStmt, idx, cTypeHint, sqlTypeHint, column, len, cid);
}

SQLRETURN ColBinder::Fetch(SQLHSTMT hStmt)
{
    return SQLFetch(hStmt);
}
