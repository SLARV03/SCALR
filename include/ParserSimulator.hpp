#ifndef PARSERSIMULATOR_HPP
#define PARSERSIMULATOR_HPP

#include "Shared_Structs.hpp"
#include <vector>
#include <string>

struct ParseStep {
    std::string stackStr;
    std::string inputStr;
    std::string actionStr;
};

struct ParseResult {
    bool accepted;
    std::vector<ParseStep> steps;
    std::string errorMsg;
};

class ParserSimulator {
public:
    static ParseResult simulate(const std::vector<Token>& tokens, const ParsingTable& table, const Grammar& grammar);
};

#endif // PARSERSIMULATOR_HPP
