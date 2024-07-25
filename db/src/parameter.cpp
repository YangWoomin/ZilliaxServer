
#include    "db/parameter.h"
#include    "internal_common.h"

using namespace zs::common;
using namespace zs::db;

template <typename T>
bool bindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, T& param, SQLLenSPtr& len, contextID cid)
{
    SQLRETURN ret = SQLBindParameter(
        hStmt, idx + 1, 
        static_cast<int32_t>(type), cTypeHint, sqlTypeHint, 
        *len, 0, (SQLPOINTER)&param, *len, len.get());
    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(hStmt, SQL_HANDLE_STMT, "bindParam failed, ret : %d, idx : %lu, param type : %d, c type : %d, sql type : %d, len : %lld, cid : %llu", ret, idx + 1, type, cTypeHint, sqlTypeHint, *len, cid);
        return false;
    }

    return true;
}

template<>
bool bindParam<const char*>(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, const char*& param, SQLLenSPtr& len, contextID cid)
{
    // const char* should be input type
    if (ParamType::INPUT != type)
    {
        return false;
    }
    
    *len = SQL_NTS;

    SQLRETURN ret = SQLBindParameter(
        hStmt, idx + 1, 
        static_cast<int32_t>(type), cTypeHint, sqlTypeHint, 
        strlen(param), 0, (SQLPOINTER)param, strlen(param), len.get());

    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(hStmt, SQL_HANDLE_STMT, "bindParam failed for const char*, ret : %d, idx : %lu, param type : %d, c type : %d, sql type : %d, len : %lld, cid : %llu", ret, idx + 1, type, cTypeHint, sqlTypeHint, *len, cid);
        return false;
    }

    return true;
}

template<>
bool bindParam<std::string>(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, std::string& param, SQLLenSPtr& len, contextID cid)
{
    if (0 == param.size())
    {
        param.resize(DEFAULT_BUFFER_COLUMN_LEN, NULL);
    }

    if (SQL_VARCHAR == sqlTypeHint)
    {
        *len = SQL_NTS;
    }
    else // if (SQL_BINARY == sqlTypeHint)
    {
        *len = param.size();
    }

    SQLRETURN ret = SQLBindParameter(
        hStmt, idx + 1, 
        static_cast<int32_t>(type), cTypeHint, sqlTypeHint, 
        param.size(), 0, (SQLCHAR*)param.data(), param.size(), len.get());
    if (!SQL_SUCCEEDED(ret))
    {
        HandleSQLError(hStmt, SQL_HANDLE_STMT, "bindParam failed for string, ret : %d, idx : %lu, param type : %d, c type : %d, sql type : %d, len : %lld, cid : %llu", ret, idx + 1, type, cTypeHint, sqlTypeHint, *len, cid);
        return false;
    }

    return true;
}

bool ParamBinder::BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, int8_t& param, SQLLenSPtr& len, contextID cid)
{
    return bindParam(hStmt, idx, type, cTypeHint, sqlTypeHint, param, len, cid);
}

bool ParamBinder::BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, uint8_t& param, SQLLenSPtr& len, contextID cid)
{
    return bindParam(hStmt, idx, type, cTypeHint, sqlTypeHint, param, len, cid);
}

bool ParamBinder::BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, int16_t& param, SQLLenSPtr& len, contextID cid)
{
    return bindParam(hStmt, idx, type, cTypeHint, sqlTypeHint, param, len, cid);
}

bool ParamBinder::BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, uint16_t& param, SQLLenSPtr& len, contextID cid)
{
    return bindParam(hStmt, idx, type, cTypeHint, sqlTypeHint, param, len, cid);
}

bool ParamBinder::BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, int32_t& param, SQLLenSPtr& len, contextID cid)
{
    return bindParam(hStmt, idx, type, cTypeHint, sqlTypeHint, param, len, cid);
}

bool ParamBinder::BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, uint32_t& param, SQLLenSPtr& len, contextID cid)
{
    return bindParam(hStmt, idx, type, cTypeHint, sqlTypeHint, param, len, cid);
}

bool ParamBinder::BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, int64_t& param, SQLLenSPtr& len, contextID cid)
{
    return bindParam(hStmt, idx, type, cTypeHint, sqlTypeHint, param, len, cid);
}

bool ParamBinder::BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, uint64_t& param, SQLLenSPtr& len, contextID cid)
{
    return bindParam(hStmt, idx, type, cTypeHint, sqlTypeHint, param, len, cid);
}

bool ParamBinder::BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, float32_t& param, SQLLenSPtr& len, contextID cid)
{
    return bindParam(hStmt, idx, type, cTypeHint, sqlTypeHint, param, len, cid);
}

bool ParamBinder::BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, float64_t& param, SQLLenSPtr& len, contextID cid)
{
    return bindParam(hStmt, idx, type, cTypeHint, sqlTypeHint, param, len, cid);
}

bool ParamBinder::BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, const char*& param, SQLLenSPtr& len, contextID cid)
{
    return bindParam(hStmt, idx, type, cTypeHint, sqlTypeHint, param, len, cid);
}

bool ParamBinder::BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, std::string& param, SQLLenSPtr& len, contextID cid)
{
    return bindParam(hStmt, idx, type, cTypeHint, sqlTypeHint, param, len, cid);
}

bool ParamBinder::BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, bytes_t& param, SQLLenSPtr& len, contextID cid)
{
    return bindParam(hStmt, idx, type, cTypeHint, sqlTypeHint, param, len, cid);
}
