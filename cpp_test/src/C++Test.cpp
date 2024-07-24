// C++Test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "variant_test.h"
#include "tuple_apply_test1.h"
#include "tuple_apply_test2.h"
#include "infer_type_test.h"
#include "type_and_enum_check.h"

int main()
{
    // std::cout << std::endl;
    // std::cout << "====================== variant test ======================" << std::endl;
    // variant_test();
    // std::cout << std::endl;

    // std::cout << "====================== tuple apply test 1 ======================" << std::endl;
    // tuple_apply_test1();
    // std::cout << std::endl;

    // std::cout << "====================== tuple apply test 2 ======================" << std::endl;
    // tuple_apply_test2();
    // std::cout << std::endl;

    // std::cout << "====================== infer type test ======================" << std::endl;
    // infer_type_test();
    // std::cout << std::endl;

    std::cout << "====================== type and enum check test ======================" << std::endl;
    type_and_enum_check_test();
    std::cout << std::endl;
}
