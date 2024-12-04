
#ifndef __ZS_MQ_MQ_H__
#define __ZS_MQ_MQ_H__


#if defined(_WIN64_)

#ifdef ZS_MQ_EXPORTS
#define __ZS_MQ_API __declspec(dllexport)
#else // ZS_MQ_EXPORTS
#define __ZS_MQ_API __declspec(dllimport)
#endif

#elif defined(_POSIX_) 

#ifdef ZS_MQ_EXPORTS
#define __ZS_MQ_API __attribute__((visibility ("default")))
#else // ZS_MQ_EXPORTS
#define __ZS_MQ_API 
#endif

#endif // _WIN64_

#include    "common/types.h"
#include    "common/log.h"

namespace zs
{
namespace mq
{
    using namespace zs::common;
    
    class __ZS_MQ_API MQ final
    {
    public:
        static void Initialize();

    private:

        MQ() = delete;
        ~MQ() = delete;
        MQ(const MQ&) = delete;
        MQ& operator=(const MQ&) = delete;
    };
}
}

#endif // __ZS_MQ_MQ_H__
