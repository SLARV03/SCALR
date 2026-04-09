#include "MetaParser.hpp"
#include <sstream>

Grammar MetaParser::parse(const std::string& input) {
    Grammar grammar;
    std::istringstream stream(input);
    std::string line;
    int lineNum = 0;

    //Parse each line of the gramma
    while (std::getline(stream, line)) {
        lineNum++;
        if (trim(line).empty()) continue;

        // Every grammar line must contain "->".
        // Lines without it are skipped.
        size_t arrowPos = line.find("->");
        if (arrowPos == std::string::npos) continue;

        std::string lhs = trim(line.substr(0, arrowPos));
        std::string rhsPart = line.substr(arrowPos + 2);

        // Split the RHS by '|' to get each alternative.
        std::istringstream altStream(rhsPart);
        std::string alt;
        while (std::getline(altStream, alt, '|')) {
            std::string trimmedAlt = trim(alt);

            if (trimmedAlt.empty() || trimmedAlt == "epsilon" || trimmedAlt == "ε") {
                grammar.addRule(lhs, {}, lineNum);
            } else {
                // Split the alternative by whitespace to get individual symbols.
                // IMPORTSNT: Each whitespace-separated token is one symbol.
                std::vector<std::string> symbols;
                std::istringstream tokStream(trimmedAlt);
                std::string tok;
                while (tokStream >> tok) {
                    symbols.push_back(tok);
                }
                grammar.addRule(lhs, symbols, lineNum);
            }
        }
    }

    //Set start symbol (LHS of first rule)
    if (!grammar.rules.empty()) {
        grammar.startSymbol = grammar.rules[0].lhs;
    }

    //Classify terminals vs non-terminals
    grammar.classifySymbols();

    //Augment: add S' -> S as rule 0
    grammar.augment();

    return grammar;
}

std::string MetaParser::trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}
