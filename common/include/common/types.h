
#ifndef __ZS_COMMON_TYPES_H__
#define __ZS_COMMON_TYPES_H__

#include    <cstdint>
#include    <string>

namespace zs
{
namespace common
{
    using float32_t     = float;
    using float64_t     = double;

    struct bytes_t : public std::string
    {};

    using contextID = uint64_t;
}
}

#endif // __ZS_COMMON_TYPES_H__
