#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <utility>
#include "MetaParser.hpp"
#include "SetGenerator.hpp"
#include "LR0.hpp"
#include "SLR1.hpp"
#include "CLR1.hpp"
#include "LALR1.hpp"
#include "JsonExporter.hpp"
#include "Lexer.hpp"
#include "ParserSimulator.hpp"
#include "ExampleGenerator.hpp"

int main(int argc, char* argv[]) {
    // Read grammar from stdin
    std::string input;
    std::string inputStr;
    std::string line;
    bool parsingInput = false;
    while (std::getline(std::cin, line)) {
        if (line.substr(0, 3) == "---") {
            parsingInput = true;
            continue;
        }
        if (parsingInput) {
            inputStr += line + "\n";
        } else {
            input += line + "\n";
        }
    }
    
    if (input.empty() || input.find_first_not_of(" \t\r\n") == std::string::npos) {
        std::cout << "{\"status\":\"error\",\"message\":\"No grammar input provided\"}" << std::endl;
        return 1;
    }
    
    // Parse grammar (also augments and classifies symbols)
    Grammar g = MetaParser::parse(input);
    
    // Compute FIRST and FOLLOW sets
    SetGenerator::computeFirst(g);
    SetGenerator::computeFollow(g);
    
    std::vector<Token> tokens;
    if (parsingInput) {
        tokens = Lexer::tokenize(inputStr, g);
    }
    
    std::vector<std::pair<std::string, ParsingTable>> allTables;
    std::vector<double> allTimes;
    std::vector<std::vector<ExampleString>> allExamples;
    std::vector<ParseResult> allParseResults;
    
    const std::string methods[] = {"LR0", "SLR1", "LALR1", "CLR1"};
    
    for (const auto& m : methods) {
        auto t1 = std::chrono::high_resolution_clock::now();
        
        ParsingTable pt;
        if (m == "LR0")       pt = LR0::generate(g);
        else if (m == "SLR1") pt = SLR1::generate(g);
        else if (m == "CLR1") pt = CLR1::generate(g);
        else if (m == "LALR1") pt = LALR1::generate(g);
        
        auto t2 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        
        allTables.push_back({m, pt});
        allTimes.push_back(ms);
        allExamples.push_back(ExampleGenerator::generate(pt, g));
        if (parsingInput) {
            allParseResults.push_back(ParserSimulator::simulate(tokens, pt, g));
        }
    }
    
    // Output all methods JSON to stdout (for GUI consumption)
    std::string allJson = JsonExporter::exportAllToJSON(allTables, g, allTimes, &allExamples, parsingInput ? &allParseResults : nullptr);
    std::cout << allJson << std::endl;
    
    // We can also overwrite output.json for debug purposes
    std::ofstream outFile("output.json");
    if (outFile.is_open()) {
        outFile << allJson << std::endl;
        outFile.close();
    }
    
    return 0;
}
