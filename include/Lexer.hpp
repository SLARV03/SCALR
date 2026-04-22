#ifndef LEXER_HPP
#define LEXER_HPP

#include "Shared_Structs.hpp"
#include <string>
#include <vector>

class Lexer {
public:
    static std::vector<Token> tokenize(const std::string& input, const Grammar& grammar);
};

#endif // LEXER_HPP
