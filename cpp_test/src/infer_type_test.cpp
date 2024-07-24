#include <iostream>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <cstdint>

using float32_t = float;
using float64_t = double;

class TestClass
{
public:
    TestClass(const std::string& val) : _val(val)
    {}

private:
    std::string _val;

    friend std::ostream& operator<<(std::ostream& os, const TestClass& tc);
};

std::ostream& operator<<(std::ostream& os, const TestClass& tc) {
    os << tc._val;
    return os;
}

// Helper to print the type
template<typename T>
void print_type(const T& value) {
    std::cout << "Type: " << typeid(T).name() << ", Value: " << value << std::endl;
}

// Variadic template function
template<typename... Args>
void process_args(const Args&... args) {
    auto tuple = std::make_tuple(args...);
    std::apply([](const auto&... item) { ((print_type(item)), ...); }, tuple);
}

int infer_type_test() {
#if defined(__GNUC__) || defined(__GNUG__)
    process_args(1, 3.14, "Hello", std::string("World"));
#endif // __GNUC__ ||  __GNUG__
    std::cout << std::endl;
    std::cout << std::endl;
    process_args(
        (char)1, 
        (signed char)2, 
        (unsigned char)3,
        (short)4,
        (unsigned short)5,
        (int)6,
        (unsigned int)7,
        (long)8,
        (unsigned long)9,
        (long long)10,
        (unsigned long long)11,
        (float32_t)1.1, 
        (float64_t)11.111, 
        (const char*)"const char* test123", 
        std::string("std string test321"),
        (bool)true,
        (int8_t)1,
        (uint8_t)2,
        (int16_t)3,
        (uint16_t)4,
        (int32_t)5,
        (uint32_t)6,
        (int64_t)7,
        (uint64_t)8,
        (TestClass)TestClass{"testclass"}
    );

    if (typeid(uint8_t) == typeid(unsigned char))
    {
        std::cout << "typeid(uint8_t) == typeid(unsigned char) equals" << std::endl;
    }
    else
    {
        std::cout << "typeid(uint8_t) == typeid(unsigned char) not equals" << std::endl;
    }
    
    return 0;
}
