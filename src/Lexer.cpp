#include "Lexer.hpp"
#include <cctype>

std::vector<Token> Lexer::tokenize(const std::string& input, const Grammar& grammar) {
    std::vector<Token> tokens;
    int line = 1;
    int col = 1;
    size_t pos = 0;

    // Collect all terminals, sorted by length descending so longer tokens match first (e.g., "id" before "i")
    std::vector<std::string> terminals(grammar.terminals.begin(), grammar.terminals.end());
    std::sort(terminals.begin(), terminals.end(), [](const std::string& a, const std::string& b) {
        if (a.length() != b.length()) return a.length() > b.length();
        return a < b;
    });

    while (pos < input.length()) {
        char ch = input[pos];
        
        if (std::isspace(ch)) {
            if (ch == '\n') {
                line++;
                col = 1;
            } else {
                col++;
            }
            pos++;
            continue;
        }

        bool matched = false;
        for (const auto& term : terminals) {
            // "epsilon" or "ε" isn't matched in the input stream usually
            if (term == "epsilon" || term == "ε") continue;
            
            if (input.compare(pos, term.length(), term) == 0) {
                tokens.push_back({term, term, line, col});
                pos += term.length();
                col += term.length();
                matched = true;
                break;
            }
        }

        if (!matched) {
            // Unrecognized character
            std::string unknownStr(1, ch);
            tokens.push_back({"error", unknownStr, line, col});
            pos++;
            col++;
        }
    }
    
    // Always append end-of-input token
    tokens.push_back({"$", "$", line, col});
    return tokens;
}
