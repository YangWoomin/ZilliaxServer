
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

    using Script = std::string;
    using Keys = std::vector<std::string>;
    using ContextID = uint64_t;
    using WorkerHash = uint64_t;
    using Args = std::vector<std::string>;
    using ResultSet1 = std::vector<std::string>;
    using SimpleResult = long long;
    using AsyncSet1Callback = std::function<void(ContextID, Keys&&, Args&&, bool, ResultSet1&&)>;
    using AsyncSet2Callback = std::function<void(ContextID, Keys&&, Args&&, bool, SimpleResult)>;

    class __ZS_CACHE_API Cache final
    {
    public:
        static bool Initialize(Logger::Messenger msgr, const std::string& dsn, int32_t workerCount);
        static void Finalize();

        static bool Run(const Script& script, ContextID cid, Keys&& keys, Args&& args, WorkerHash wh, AsyncSet1Callback cb);
        static bool Run(const Script& script, ContextID cid, Keys&& keys, Args&& args, WorkerHash wh, AsyncSet2Callback cb);
        static bool Run(const Script& script, ContextID cid, Keys&& keys, Args&& args, ResultSet1& rs);
        static bool Run(const Script& script, ContextID cid, Keys&& keys, Args&& args, SimpleResult& rs);

    private:

        Cache() = delete;
        ~Cache() = delete;
        Cache(const Cache&) = delete;
        Cache& operator=(const Cache&) = delete;
    };
}
}

#endif // __ZS_CACHE_CACHE_H__
