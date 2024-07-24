#include <iostream>
#include <tuple>
#include <utility>

#include "tuple_apply_test1.h"

// Helper function to call a lambda for each element in the tuple
template <typename Tuple, typename Func, std::size_t... Indices>
void apply_impl(const Tuple& t, Func f, std::index_sequence<Indices...>) {
    (f(std::get<Indices>(t)), ...);
}

template <typename Tuple, typename Func>
void for_each_in_tuple(const Tuple& t, Func f) {
    apply_impl(t, f, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

int tuple_apply_test1() {
    auto myTuple = std::make_tuple(1, 2.5, std::string("Hello"));

    auto printElement = [](const auto& element) {
        std::cout << element << std::endl;
        };

    for_each_in_tuple(myTuple, printElement);

    return 0;
}
