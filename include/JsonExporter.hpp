#ifndef JSONEXPORTER_HPP
#define JSONEXPORTER_HPP

#include "Shared_Structs.hpp"
#include "ParserSimulator.hpp"
#include "ExampleGenerator.hpp"
#include "Lexer.hpp"
#include <string>
#include <vector>
#include <utility>

class JsonExporter {
public:
    // Single parser export (used by GUI via stdout)
    static std::string exportToJSON(const ParsingTable& table, const Grammar& grammar, double timeMs, 
                                    const ParseResult* parseResult = nullptr,
                                    const std::vector<ExampleString>* examples = nullptr);
    
    // All parsers unified export
    static std::string exportAllToJSON(
        const std::vector<std::pair<std::string, ParsingTable>>& tables,
        const Grammar& grammar,
        const std::vector<double>& times,
        const std::vector<std::vector<ExampleString>>* allExamples = nullptr,
        const std::vector<ParseResult>* allParseResults = nullptr);
};

#endif
