
#ifndef __ZS_COMMON_UTILS_H__
#define __ZS_COMMON_UTILS_H__

#include    <tuple>

namespace zs
{
namespace common
{
    // tuple interator
    template <typename Tuple, typename Func, std::size_t... Indices>
    bool ApplyTupleElement(Tuple& t, Func f, std::index_sequence<Indices...>)
    {
        return (f(Indices, std::get<Indices>(t)) && ...);
    }

    template <typename Tuple, typename Func>
    bool ApplyTuple(Tuple& t, Func f)
    {
        return ApplyTupleElement(t, f, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
    }

    // check all conditions are true
    template <bool...>
    struct bool_pack;

    template <bool... BS>
    struct all_true : std::is_same<bool_pack<true, BS...>, bool_pack<BS..., true>> {};
}
}

#endif // __ZS_COMMON_UTILS_H__
