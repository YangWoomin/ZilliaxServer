#include <iostream>
#include <tuple>
#include <type_traits>
#include <string>

// ParamType을 나타내는 enum 정의
enum class ParamType {
    Input,
    InOut,
    Output
};

// 주어진 타입과 ParamType 조합이 유효한지 확인하는 템플릿
template<typename T, ParamType P>
struct is_valid_type_param : std::true_type {};

// const char* 타입에 대한 특수화 1
template<>
struct is_valid_type_param<const char*, ParamType::Output> : std::false_type {};

// const char* 타입에 대한 특수화 2
template<>
struct is_valid_type_param<const char*, ParamType::InOut> : std::false_type {};

// 모든 조건이 true인지 확인하는 템플릿
template<bool...>
struct bool_pack;

template<bool... Bs>
struct all_true : std::is_same<bool_pack<true, Bs...>, bool_pack<Bs..., true>> {};

// 가변 인자 템플릿 클래스
template<typename... Args>
class Params {
public:
    using TupleType = std::tuple<Args...>;

    // 생성자에서 매개변수 값 저장
    Params(Args... args) : values(args...) {}

    // 타입 검사를 수행하는 가변 인자 템플릿 멤버 함수
    template<typename... Types>
    void checkAndProcess(Types... types) {
        static_assert(sizeof...(Args) == sizeof...(Types), "매개변수의 개수가 일치하지 않습니다.");
        static_assert(all_true<is_valid_type_param<Args, Types>::value...>::value, "매개변수 타입이 올바르지 않습니다.");

        // 필요한 동작 수행 (여기서는 단순히 출력)
        printTuple(values);
    }

    // 값 설정 및 출력
    void setParams(Args... args) {
        values = std::make_tuple(args...);
        // 필요한 동작 수행 (여기서는 단순히 출력)
        printTuple(values);
    }

private:
    TupleType values;

    template<std::size_t Index = 0, typename Tuple>
    void printTuple(const Tuple& tuple) {
        if constexpr (Index < std::tuple_size_v<Tuple>) {
            std::cout << std::get<Index>(tuple) << std::endl;
            printTuple<Index + 1>(tuple);
        }
    }
};

int main() {
    // 올바른 타입들로 Params 객체 생성
    Params<int, double, const char*, std::string> obj1(1, 2.5, "Hello", std::string("World"));

    // 올바른 타입과 enum 값으로 멤버 함수 호출
    obj1.checkAndProcess(ParamType::Input, ParamType::Input, ParamType::Input, ParamType::Input);

    // 잘못된 타입이 포함된 경우 컴파일 에러 발생
    // Params<int, double, const char*, std::string> obj2(1, 2.5, "Hello", std::string("World"));
    // obj2.checkAndProcess(ParamType::Input, ParamType::Input, ParamType::Output, ParamType::Input); // 컴파일 에러

    return 0;
}
