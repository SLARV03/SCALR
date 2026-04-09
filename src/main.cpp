#include <iostream>
#include <string>
#include <chrono>
#include "MetaParser.hpp"
#include "SetGenerator.hpp"
#include "LR0.hpp"
#include "SLR1.hpp"
#include "CLR1.hpp"
#include "LALR1.hpp"
#include "JsonExporter.hpp"

int main(int argc, char* argv[]) {
    // Read grammar from stdin
    std::string input;
    std::string line;
    while (std::getline(std::cin, line)) {
        input += line + "\n";
    }
    
    if (input.empty() || input.find_first_not_of(" \t\r\n") == std::string::npos) {
        std::cout << "{\"status\":\"error\",\"message\":\"No grammar input provided\"}" << std::endl;
        return 1;
    }
    
    // Determine method from command-line args (default: SLR1)
    std::string method = "SLR1";
    if (argc > 1) {
        method = argv[1];
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Parse grammar (also augments and classifies symbols)
    Grammar g = MetaParser::parse(input);
    
    // Compute FIRST and FOLLOW sets
    SetGenerator::computeFirst(g);
    SetGenerator::computeFollow(g);
    
    // Generate table based on method
    ParsingTable table;
    if (method == "LR0")       table = LR0::generate(g);
    else if (method == "SLR1") table = SLR1::generate(g);
    else if (method == "CLR1") table = CLR1::generate(g);
    else if (method == "LALR1") table = LALR1::generate(g);
    else                       table = SLR1::generate(g);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    double timeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    // Export as JSON
    std::string json = JsonExporter::exportToJSON(table, g, timeMs);
    std::cout << json << std::endl;
    
    return 0;
}
