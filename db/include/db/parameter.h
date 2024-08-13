
#ifndef __ZS_DB_PARAMETER_H__
#define __ZS_DB_PARAMETER_H__

#if  defined(_WIN64_)
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

    enum class ParamType
    {
        INPUT            = SQL_PARAM_INPUT,
        INOUT            = SQL_PARAM_INPUT_OUTPUT,
        OUTPUT           = SQL_PARAM_OUTPUT,
    };

    // type limitations
    template <typename T, ParamType P>
    struct is_valid_type_param : std::true_type {};

    // const char* is not allowed for INOUT and OUTPUT
    template<>
    struct is_valid_type_param<const char*, ParamType::INOUT> : std::false_type {};
    template<>
    struct is_valid_type_param<const char*, ParamType::OUTPUT> : std::false_type {};

    class __ZS_DB_API ParamBinder
    {
    public:
        static bool BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, int8_t& param, SQLLenSPtr& len, contextID cid);
        static bool BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, uint8_t& param, SQLLenSPtr& len, contextID cid);
        static bool BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, int16_t& param, SQLLenSPtr& len, contextID cid);
        static bool BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, uint16_t& param, SQLLenSPtr& len, contextID cid);
        static bool BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, int32_t& param, SQLLenSPtr& len, contextID cid);
        static bool BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, uint32_t& param, SQLLenSPtr& len, contextID cid);
        static bool BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, int64_t& param, SQLLenSPtr& len, contextID cid);
        static bool BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, uint64_t& param, SQLLenSPtr& len, contextID cid);
        static bool BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, float32_t& param, SQLLenSPtr& len, contextID cid);
        static bool BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, float64_t& param, SQLLenSPtr& len, contextID cid);
        static bool BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, const char*& param, SQLLenSPtr& len, contextID cid);
        static bool BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, std::string& param, SQLLenSPtr& len, contextID cid);
        static bool BindParam(SQLHSTMT hStmt, std::size_t idx, ParamType type, int32_t cTypeHint, int32_t sqlTypeHint, bytes_t& param, SQLLenSPtr& len, contextID cid);
    };

    class IParams
    {
    public:
        virtual ~IParams() = default;

    private:
        virtual bool bindParams(SQLHSTMT hStmt, contextID cid) = 0;

    friend class Operation;
    };

    template <typename... Args>
    class Params final : public IParams
    {
    public:
        Params(Args... args)
            : _params(std::make_tuple(args...))
        {
            
        }

        void Reset(Args... args)
        {
            _params = std::make_tuple(args...);
        }

        template <ParamType... Types>
        void SetParamTypes()
        {
            static_assert(sizeof...(Args) == sizeof...(Types), 
                "Number of arguments and param types must match");

            static_assert(all_true<is_valid_type_param<Args, Types>::value...>::value,
                "Invalid ParmaType such as const char* for inout or output");

            _types = {Types...};
        }

        using ParamsRaw = std::tuple<Args...>;

        const ParamsRaw& GetParams()
        {
            return _params;
        }

        ParamType GetType(std::size_t idx) const
        {
            return _types[idx];
        }

    private:
        virtual bool bindParams(SQLHSTMT hStmt, contextID cid) override final
        {
            // bind params using SQLBindParameter
            auto binder = [this, hStmt, cid](std::size_t idx, auto& ele)  
            {
                auto cTypeHinter = std::get<SQL_C_TYPE_IDX>(_hinter);
                auto sqlTypeHinter = std::get<SQL_SQL_TYPE_IDX>(_hinter);
                ParamType type = GetType(idx);
                SQLLenSPtr len = std::make_shared<SQLLEN>(0);

                if (false == ParamBinder::BindParam(
                    hStmt, idx, type, cTypeHinter[idx], sqlTypeHinter[idx],
                    ele, len, cid))
                {
                    return false;
                }

                _lens.push_back(len);

                return true;
            };

            return ApplyTuple(_params, binder);
        }

        std::tuple<Args...>                                         _params;
        std::vector<ParamType>                                      _types;
        std::vector<SQLLenSPtr>                                     _lens;
        const decltype(CreateSQLTypeHinter<Args...>())              _hinter { CreateSQLTypeHinter<Args...>() };
    };
}
}

#endif // __ZS_DB_PARAMETER_H__
