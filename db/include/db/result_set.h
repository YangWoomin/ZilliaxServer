
#ifndef __ZS_DB_RESULT_SET_H__
#define __ZS_DB_RESULT_SET_H__

#if  defined(_MSVC_)
#include    <windows.h>
#endif

#include    <sql.h>
#include    <sqlext.h>
#include	<sqltypes.h>

#include    <string>
#include    <vector>

#include    "common/types.h"
#include    "common/utils.h"
#include    "db/common.h"

namespace zs
{
namespace db
{
    using namespace zs::common;

    bool __ZS_DB_API BindCols(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, int8_t& column, SQLLenSPtr& len, contextID cid);
    bool __ZS_DB_API BindCols(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, uint8_t& column, SQLLenSPtr& len, contextID cid);
    bool __ZS_DB_API BindCols(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, int16_t& column, SQLLenSPtr& len, contextID cid);
    bool __ZS_DB_API BindCols(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, uint16_t& column, SQLLenSPtr& len, contextID cid);
    bool __ZS_DB_API BindCols(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, int32_t& column, SQLLenSPtr& len, contextID cid);
    bool __ZS_DB_API BindCols(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, uint32_t& column, SQLLenSPtr& len, contextID cid);
    bool __ZS_DB_API BindCols(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, int64_t& column, SQLLenSPtr& len, contextID cid);
    bool __ZS_DB_API BindCols(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, uint64_t& column, SQLLenSPtr& len, contextID cid);
    bool __ZS_DB_API BindCols(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, float32_t& column, SQLLenSPtr& len, contextID cid);
    bool __ZS_DB_API BindCols(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, float64_t& column, SQLLenSPtr& len, contextID cid);
    bool __ZS_DB_API BindCols(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, std::string& column, SQLLenSPtr& len, contextID cid);
    bool __ZS_DB_API BindCols(SQLHSTMT hStmt, std::size_t idx, int32_t cTypeHint, int32_t sqlTypeHint, bytes_t& column, SQLLenSPtr& len, contextID cid);
    SQLRETURN __ZS_DB_API Fetch(SQLHSTMT hStmt);
    
    class IResultSet
    {
    public:
        virtual ~IResultSet() = default;

    private:
        virtual bool bindColumns(SQLHSTMT hStmt, contextID cid) = 0;
        virtual void resetColmns() = 0;
        virtual void fetch(SQLHSTMT hStmt) = 0;

    friend class Operation;
    };

    template <typename... Args>
    class ResultSet final : public IResultSet
    {
    public:
        ResultSet()
            : _cols(std::make_tuple(Args()...))
        {}

        using Result = std::vector<std::tuple<Args...>>;

        const Result& GetResult()
        {
            return _rs;
        }

    private:
        virtual bool bindColumns(SQLHSTMT hStmt, contextID cid) override final
        {
            // bind columns using SQLBindCol
            auto bindSQLCols = [this, hStmt, cid](std::size_t idx, auto& ele) 
            {
                auto cTypeHinter = std::get<SQL_C_TYPE_IDX>(_hinter);
                auto sqlTypeHinter = std::get<SQL_SQL_TYPE_IDX>(_hinter);
                SQLLenSPtr len = std::make_shared<SQLLEN>(0);

                if (false == BindCol(
                    hStmt, idx, cTypeHinter[idx], sqlTypeHinter[idx],
                    ele, len, cid
                ))
                {
                    return false;
                }

                _lens.push_back(len);

                return true;
            };

            return ApplyTuple(_cols, bindSQLCols);
        }

        virtual void resetColumns() override final
        {
            auto resetCols = [this](std::size_t idx, auto& ele) 
            {
                auto sqlTypeHinter = std::get<SQL_SQL_TYPE_IDX>(_hinter);
                if (SQL_VARCHAR == sqlTypeHinter[idx])
                {
                    // we know ele type explicitly
                    std::string& val = (std::string&)ele;
                    std::fill(val.begin(), val.end(), NULL);
                }
                else if (SQL_BINARY == sqlTypeHinter[idx])
                {
                    // we know ele type explicitly
                    bytes_t& bytes = (bytes_t&)ele;
                    std::fill(bytes.begin(), bytes.end(), NULL);
                }
                else
                {
                    // others (primitives)
                    ele = 0;
                }

                return true;
            };

            ApplyTuple(_cols, resetCols);
        }

        virtual void fetch(SQLHSTMT hStmt) override final
        {
            SQLRETURN res = Fetch(hStmt);
            while (SQL_NO_DATA != res && SQL_SUCCEEDED(res))
            {
                _rs.push_back(_cols);
                resetColumns();
            }
        }

        std::tuple<Args...>                                         _cols;
        std::vector<SQLLenSPtr>                                     _lens;
        Result                                                      _rs;
        const decltype(CreateSQLTypeHinter<Args...>())              _hinter;
    };
}
}

#endif // __ZS_DB_RESULT_SET_H__
