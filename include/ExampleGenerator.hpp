#ifndef EXAMPLEGENERATOR_HPP
#define EXAMPLEGENERATOR_HPP

#include "Shared_Structs.hpp"
#include <vector>
#include <string>

struct ExampleString {
    std::string string;
    std::string type; // "conflict" or "normal"
    std::string description;
};

class ExampleGenerator {
public:
    static std::vector<ExampleString> generate(const ParsingTable& table, const Grammar& grammar);
};

#endif // EXAMPLEGENERATOR_HPP
