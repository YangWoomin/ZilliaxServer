
#ifndef __ZS_DB_COMMON_H__
#define __ZS_DB_COMMON_H__

#if defined(_WIN64_)

#ifdef ZS_DB_EXPORTS
#define __ZS_DB_API __declspec(dllexport)
#else // ZS_DB_EXPORTS
#define __ZS_DB_API __declspec(dllimport)
#endif

#elif defined(_POSIX_)

#ifdef ZS_DB_EXPORTS
#define __ZS_DB_API __attribute__((visibility ("default")))
#else // ZS_DB_EXPORTS
#define __ZS_DB_API 
#endif

#endif // _MSVC_

#if  defined(_WIN64_)
#include    <windows.h>
#endif

#include    <sql.h>
#include    <sqlext.h>
#include	<sqltypes.h>

#include    <memory>
#include    <string>
#include    <array>

#include    "common/types.h"
#include    "common/log.h"

namespace zs
{
namespace db
{
    using namespace zs::common;

    class Operation;
    using OperationSPtr = std::shared_ptr<Operation>;

    class Worker;
    using WorkerSPtr = std::shared_ptr<Worker>;

    class IParams;
    using ParamsSPtr = std::shared_ptr<IParams>;

    class IResultSet;
    using ResultSetSPtr = std::shared_ptr<IResultSet>;

    static const std::size_t DEFAULT_BUFFER_COLUMN_LEN = 256;

    using SQLLenSPtr = std::shared_ptr<SQLLEN>;

    enum class TransIsolLevel
    {
        READ_UNCOMMITTED        = SQL_TXN_READ_UNCOMMITTED,
        READ_COMMITTED          = SQL_TXN_READ_COMMITTED,
        REPEATABLE_READ         = SQL_TXN_REPEATABLE_READ,
        SERIALIZABLE            = SQL_TXN_SERIALIZABLE
    };

    struct Config
    {
        std::string     _dsn;
        std::size_t     _workerCnt;
        uint16_t        _connTimeout;
        uint16_t        _stmtTimeout;
        uint32_t        _connTryInterval;
        uint32_t        _connTryCount;
        TransIsolLevel  _isolLevel = TransIsolLevel::REPEATABLE_READ;
    };

    template <typename T>
    struct SQLTypeHinter;

    template <>
    struct SQLTypeHinter<int8_t>
    {
        static constexpr int32_t cType          = SQL_C_STINYINT;
        static constexpr int32_t sqlType        = SQL_TINYINT;
    };

    template <>
    struct SQLTypeHinter<uint8_t>
    {
        static constexpr int32_t cType          = SQL_C_UTINYINT;
        static constexpr int32_t sqlType        = SQL_TINYINT;
    };

    template <>
    struct SQLTypeHinter<int16_t>
    {
        static constexpr int32_t cType          = SQL_C_SSHORT;
        static constexpr int32_t sqlType        = SQL_SMALLINT;
    };

    template <>
    struct SQLTypeHinter<uint16_t>
    {
        static constexpr int32_t cType          = SQL_C_USHORT;
        static constexpr int32_t sqlType        = SQL_SMALLINT;
    };

    template <>
    struct SQLTypeHinter<int32_t>
    {
        static constexpr int32_t cType          = SQL_C_SLONG;
        static constexpr int32_t sqlType        = SQL_INTEGER;
    };

    template <>
    struct SQLTypeHinter<uint32_t>
    {
        static constexpr int32_t cType          = SQL_C_ULONG;
        static constexpr int32_t sqlType        = SQL_INTEGER;
    };

    template <>
    struct SQLTypeHinter<int64_t>
    {
        static constexpr int32_t cType          = SQL_C_SBIGINT;
        static constexpr int32_t sqlType        = SQL_BIGINT;
    };

    template <>
    struct SQLTypeHinter<uint64_t>
    {
        static constexpr int32_t cType          = SQL_C_UBIGINT;
        static constexpr int32_t sqlType        = SQL_BIGINT;
    };

    template <>
    struct SQLTypeHinter<float32_t>
    {
        static constexpr int32_t cType          = SQL_C_FLOAT;
        static constexpr int32_t sqlType        = SQL_FLOAT;
    };

    template <>
    struct SQLTypeHinter<float64_t>
    {
        static constexpr int32_t cType          = SQL_C_DOUBLE;
        static constexpr int32_t sqlType        = SQL_DOUBLE;
    };

    template <>
    struct SQLTypeHinter<std::string>
    {
        static constexpr int32_t cType          = SQL_C_CHAR;
        static constexpr int32_t sqlType        = SQL_VARCHAR;
    };

    template <>
    struct SQLTypeHinter<const char*>
    {
        static constexpr int32_t cType          = SQL_C_CHAR;
        static constexpr int32_t sqlType        = SQL_VARCHAR;
    };

    template <>
    struct SQLTypeHinter<bytes_t>
    {
        static constexpr int32_t cType          = SQL_C_BINARY;
        static constexpr int32_t sqlType        = SQL_BINARY;
    };

    template <typename... Args>
    constexpr auto CreateSQLTypeHinter() {
        return std::make_tuple(
            std::array<int32_t, sizeof...(Args)>{ SQLTypeHinter<Args>::cType... },
            std::array<int32_t, sizeof...(Args)>{ SQLTypeHinter<Args>::sqlType... }
        );
    }

    static const std::size_t SQL_C_TYPE_IDX = 0;
    static const std::size_t SQL_SQL_TYPE_IDX = 1;
}
}


#endif // __ZS_DB_COMMON_H__
