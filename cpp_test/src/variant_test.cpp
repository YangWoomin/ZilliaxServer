#include <iostream>
#include <variant>
#include <string>

#include "variant_test.h"

int variant_test() {
	// Define a variant that can hold either an int, a float, or a std::string
	std::variant<int, float, std::string> myVariant;

	// Assign an int value to the variant
	myVariant = 10;
	std::cout << "Int: " << std::get<int>(myVariant) << std::endl;

	// Assign a float value to the variant
	myVariant = 3.14f;
	std::cout << "Float: " << std::get<float>(myVariant) << std::endl;

	// Assign a string value to the variant
	myVariant = std::string("Hello, Variant!");
	std::cout << "String: " << std::get<std::string>(myVariant) << std::endl;

	// Using std::holds_alternative to check the current type
	if (std::holds_alternative<int>(myVariant)) {
		std::cout << "The variant holds an int." << std::endl;
	}
	else if (std::holds_alternative<float>(myVariant)) {
		std::cout << "The variant holds a float." << std::endl;
	}
	else if (std::holds_alternative<std::string>(myVariant)) {
		std::cout << "The variant holds a string." << std::endl;
	}

	// Using std::visit to process the variant value
	std::visit([](auto&& arg) {
		std::cout << "Visited value: " << arg << std::endl;
		}, myVariant);

	return 0;
}
