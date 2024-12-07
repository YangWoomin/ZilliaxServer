
#ifndef __ZS_CACHE_CACHE_H__
#define __ZS_CACHE_CACHE_H__


#if defined(_WIN64_)

#ifdef ZS_CACHE_EXPORTS
#define __ZS_CACHE_API __declspec(dllexport)
#else // ZS_CACHE_EXPORTS
#define __ZS_CACHE_API __declspec(dllimport)
#endif

#elif defined(_POSIX_) 

#ifdef ZS_CACHE_EXPORTS
#define __ZS_CACHE_API __attribute__((visibility ("default")))
#else // ZS_CACHE_EXPORTS
#define __ZS_CACHE_API 
#endif

#endif // _WIN64_

#include    "common/types.h"
#include    "common/log.h"

#include    <functional>
#include    <memory>
#include    <unordered_map>
#include    <vector>

namespace zs
{
namespace cache
{
    using namespace zs::common;

    class __ZS_CACHE_API Cache final
    {
    public:
        static bool Initialize(Logger::Messenger msgr, const std::string& dsn);
        static void Finalize();

        static bool Set(const std::string& script, const std::vector<std::string>& keys, const std::vector<std::string>& args);

    private:

        Cache() = delete;
        ~Cache() = delete;
        Cache(const Cache&) = delete;
        Cache& operator=(const Cache&) = delete;
    };
}
}

#endif // __ZS_CACHE_CACHE_H__
