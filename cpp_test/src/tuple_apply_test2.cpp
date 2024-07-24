#include <iostream>
#include <tuple>
#include <utility>
#include <cstdint>

#include "tuple_apply_test2.h"

void Print(std::size_t idx, int32_t& val)
{
    std::cout << "Element : " << idx << ", Value : " << val << std::endl;
}

void Print(std::size_t idx, float val)
{
    std::cout << "Element : " << idx << ", Value : " << val << std::endl;
}

void Print(std::size_t idx, std::string& val)
{
    std::cout << "Element : " << idx << ", Value : " << val << std::endl;
}

// Helper function to call a lambda for each element in the tuple along with its index
template <typename Tuple, typename Func, std::size_t... Indices>
bool apply_impl(Tuple& t, Func f, std::index_sequence<Indices...>) {
    // Fold expression to apply the function to each element and check the return value
    return (f(Indices, std::get<Indices>(t)) && ...);
}

template <typename Tuple, typename Func>
bool for_each_in_tuple(Tuple& t, Func f) {
    // Generate an index sequence of the same size as the tuple
    return apply_impl(t, f, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

int tuple_apply_test2() {
    auto myTuple = std::make_tuple(1, 2.5, std::string("Hello"));

    auto printElementWithIndex = [](std::size_t index, auto& element) {
            //std::cout << "Element " << index << ": " << element << std::endl;
            Print(index, element);
            return index != 1; // Continue iterating unless the index is 1
        };

    for_each_in_tuple(myTuple, printElementWithIndex);

    return 0;
}
