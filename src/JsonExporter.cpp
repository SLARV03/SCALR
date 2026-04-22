#include "JsonExporter.hpp"
#include <sstream>
#include <chrono>

static std::string escapeJSON(const std::string& s) {
    std::string res;
    for (char c : s) {
        if (c == '"') res += "\\\"";
        else if (c == '\\') res += "\\\\";
        else if (c == '\n') res += "\\n";
        else res += c;
    }
    return res;
}

static void appendSharedGrammarData(std::stringstream& ss, const Grammar& grammar) {
    // Grammar Map (skip augmented rule 0)
    ss << "  \"grammar_map\": [\n";
    bool first = true;
    for (const auto& rule : grammar.rules) {
        if (rule.lhs == grammar.augmentedStart) continue; // skip S' -> S
        if (!first) ss << ",\n";
        ss << "    { \"id\": " << rule.id
           << ", \"rule\": \"" << rule.toString()
           << "\", \"line\": " << rule.sourceLine << " }";
        first = false;
    }
    ss << "\n  ],\n";
    
    // First/Follow Sets (bonus diagnostic info)
    ss << "  \"first_sets\": {\n";
    first = true;
    for (const auto& nt : grammar.nonTerminals) {
        if (nt == grammar.augmentedStart) continue;
        if (!first) ss << ",\n";
        ss << "    \"" << nt << "\": [";
        auto it = grammar.firstSets.find(nt);
        if (it != grammar.firstSets.end()) {
            bool f2 = true;
            for (const auto& s : it->second) {
                if (!f2) ss << ", ";
                ss << "\"" << s << "\"";
                f2 = false;
            }
        }
        ss << "]";
        first = false;
    }
    ss << "\n  },\n";
    
    ss << "  \"follow_sets\": {\n";
    first = true;
    for (const auto& nt : grammar.nonTerminals) {
        if (nt == grammar.augmentedStart) continue;
        if (!first) ss << ",\n";
        ss << "    \"" << nt << "\": [";
        auto it = grammar.followSets.find(nt);
        if (it != grammar.followSets.end()) {
            bool f2 = true;
            for (const auto& s : it->second) {
                if (!f2) ss << ", ";
                ss << "\"" << s << "\"";
                f2 = false;
            }
        }
        ss << "]";
        first = false;
    }
    ss << "\n  },\n";
}

// Helper: export a single parser's table/conflicts block
static void exportParserBlock(std::stringstream& ss, const std::string& method,
                               const ParsingTable& table, double timeMs,
                               const Grammar& grammar,
                               const std::vector<ExampleString>* examples = nullptr,
                               const ParseResult* parseResult = nullptr) {
    int numStates = table.numStates;
    if (numStates == 0) {
        for (const auto& [k, v] : table.actionTable) if (k >= numStates) numStates = k + 1;
        for (const auto& [k, v] : table.gotoTable)   if (k >= numStates) numStates = k + 1;
    }

    ss << "    \"" << method << "\": {\n";
    ss << "      \"meta\": { \"states\": " << numStates
       << ", \"conflicts\": " << table.conflicts.size()
       << ", \"time_ms\": " << timeMs << " },\n";

    // Table
    ss << "      \"table\": {\n";
    bool firstState = true;
    for (int i = 0; i < numStates; ++i) {
        bool hasAction = table.actionTable.count(i) && !table.actionTable.at(i).empty();
        bool hasGoto = table.gotoTable.count(i) && !table.gotoTable.at(i).empty();
        if (!hasAction && !hasGoto) continue;

        if (!firstState) ss << ",\n";
        ss << "        \"state_" << i << "\": { ";

        bool firstEntry = true;
        if (hasAction) {
            for (const auto& [sym, actions] : table.actionTable.at(i)) {
                if (!firstEntry) ss << ", ";
                ss << "\"" << sym << "\": [";
                for (size_t k = 0; k < actions.size(); ++k) {
                    if (k > 0) ss << ", ";
                    ss << "\"" << actions[k].toString() << "\"";
                }
                ss << "]";
                firstEntry = false;
            }
        }
        if (hasGoto) {
            for (const auto& [sym, target] : table.gotoTable.at(i)) {
                if (!firstEntry) ss << ", ";
                ss << "\"" << sym << "\": [\"" << target << "\"]";
                firstEntry = false;
            }
        }
        ss << " }";
        firstState = false;
    }
    ss << "\n      },\n";

    // Conflicts
    ss << "      \"conflicts\": [\n";
    for (size_t i = 0; i < table.conflicts.size(); ++i) {
        const auto& c = table.conflicts[i];
        ss << "        { \"type\": \"" << c.type << "\", \"state\": " << c.state
           << ", \"symbol\": \"" << c.symbol << "\", \"rules\": [";
        for (size_t k = 0; k < c.rules.size(); ++k) {
            if (k > 0) ss << ", ";
            ss << c.rules[k];
        }
        ss << "] }";
        if (i < table.conflicts.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "      ]";
    
    if (examples != nullptr) {
        ss << ",\n      \"example_strings\": [\n";
        for (size_t i = 0; i < examples->size(); ++i) {
            const auto& ex = (*examples)[i];
            ss << "        { \"string\": \"" << escapeJSON(ex.string) << "\", \"type\": \"" << escapeJSON(ex.type) << "\", \"description\": \"" << escapeJSON(ex.description) << "\" }";
            if (i < examples->size() - 1) ss << ",";
            ss << "\n";
        }
        ss << "      ]\n";
    }

    if (parseResult != nullptr) {
        ss << ",\n      \"parse_trace\": {\n";
        ss << "        \"accepted\": " << (parseResult->accepted ? "true" : "false") << ",\n";
        ss << "        \"error\": \"" << escapeJSON(parseResult->errorMsg) << "\",\n";
        ss << "        \"steps\": [\n";
        for (size_t i = 0; i < parseResult->steps.size(); ++i) {
            const auto& step = parseResult->steps[i];
            ss << "          { \"stack\": \"" << escapeJSON(step.stackStr) << "\", \"input\": \"" << escapeJSON(step.inputStr) << "\", \"action\": \"" << escapeJSON(step.actionStr) << "\" }";
            if (i < parseResult->steps.size() - 1) ss << ",";
            ss << "\n";
        }
        ss << "        ]\n";
        ss << "      }\n";
    } else {
        ss << ",\n      \"parse_trace\": null\n";
    }
    ss << "    }";
}

std::string JsonExporter::exportToJSON(const ParsingTable& table, const Grammar& grammar, double timeMs, 
                                       const ParseResult* parseResult,
                                       const std::vector<ExampleString>* examples) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"status\": \"success\",\n";
    
    int numStates = table.numStates;
    // Fallback: scan keys
    if (numStates == 0) {
        for (const auto& [k, v] : table.actionTable) if (k >= numStates) numStates = k + 1;
        for (const auto& [k, v] : table.gotoTable)   if (k >= numStates) numStates = k + 1;
    }

    ss << "  \"meta\": { \"states\": " << numStates
       << ", \"conflicts\": " << table.conflicts.size()
       << ", \"time_ms\": " << timeMs << " },\n";
    
    appendSharedGrammarData(ss, grammar);
    
    // Table
    ss << "  \"table\": {\n";
    bool firstState = true;
    for (int i = 0; i < numStates; ++i) {
        bool hasAction = table.actionTable.count(i) && !table.actionTable.at(i).empty();
        bool hasGoto = table.gotoTable.count(i) && !table.gotoTable.at(i).empty();
        if (!hasAction && !hasGoto) continue;
        
        if (!firstState) ss << ",\n";
        ss << "    \"state_" << i << "\": { ";
        
        bool firstEntry = true;
        if (hasAction) {
            for (const auto& [sym, actions] : table.actionTable.at(i)) {
                if (!firstEntry) ss << ", ";
                ss << "\"" << sym << "\": [";
                for (size_t k = 0; k < actions.size(); ++k) {
                    if (k > 0) ss << ", ";
                    ss << "\"" << actions[k].toString() << "\"";
                }
                ss << "]";
                firstEntry = false;
            }
        }
        if (hasGoto) {
            for (const auto& [sym, target] : table.gotoTable.at(i)) {
                if (!firstEntry) ss << ", ";
                ss << "\"" << sym << "\": [\"" << target << "\"]";
                firstEntry = false;
            }
        }
        
        ss << " }";
        firstState = false;
    }
    ss << "\n  },\n";
    
    // Conflicts
    ss << "  \"conflicts\": [\n";
    for (size_t i = 0; i < table.conflicts.size(); ++i) {
        const auto& c = table.conflicts[i];
        ss << "    { \"type\": \"" << c.type << "\", \"state\": " << c.state
           << ", \"symbol\": \"" << c.symbol << "\", \"rules\": [";
        for (size_t k = 0; k < c.rules.size(); ++k) {
            if (k > 0) ss << ", ";
            ss << c.rules[k];
        }
        ss << "] }";
        if (i < table.conflicts.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "  ]";
    
    if (examples != nullptr) {
        ss << ",\n  \"example_strings\": [\n";
        for (size_t i = 0; i < examples->size(); ++i) {
            const auto& ex = (*examples)[i];
            ss << "    { \"string\": \"" << escapeJSON(ex.string) << "\", \"type\": \"" << escapeJSON(ex.type) << "\", \"description\": \"" << escapeJSON(ex.description) << "\" }";
            if (i < examples->size() - 1) ss << ",";
            ss << "\n";
        }
        ss << "  ]";
    }

    if (parseResult != nullptr) {
        ss << ",\n  \"parse_trace\": {\n";
        ss << "    \"accepted\": " << (parseResult->accepted ? "true" : "false") << ",\n";
        ss << "    \"error\": \"" << escapeJSON(parseResult->errorMsg) << "\",\n";
        ss << "    \"steps\": [\n";
        for (size_t i = 0; i < parseResult->steps.size(); ++i) {
            const auto& step = parseResult->steps[i];
            ss << "      { \"stack\": \"" << escapeJSON(step.stackStr) << "\", \"input\": \"" << escapeJSON(step.inputStr) << "\", \"action\": \"" << escapeJSON(step.actionStr) << "\" }";
            if (i < parseResult->steps.size() - 1) ss << ",";
            ss << "\n";
        }
        ss << "    ]\n";
        ss << "  }\n";
    } else {
        ss << ",\n  \"parse_trace\": null\n";
    }
    
    ss << "}";
    return ss.str();
}

std::string JsonExporter::exportAllToJSON(
    const std::vector<std::pair<std::string, ParsingTable>>& tables,
    const Grammar& grammar,
    const std::vector<double>& times,
    const std::vector<std::vector<ExampleString>>* allExamples,
    const std::vector<ParseResult>* allParseResults) {
    
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"status\": \"success\",\n";

    appendSharedGrammarData(ss, grammar);

    // All parsers
    ss << "  \"parsers\": {\n";
    for (size_t i = 0; i < tables.size(); ++i) {
        if (i > 0) ss << ",\n";
        const std::vector<ExampleString>* exPtr = (allExamples != nullptr && i < allExamples->size()) ? &(*allExamples)[i] : nullptr;
        const ParseResult* prPtr = (allParseResults != nullptr && i < allParseResults->size()) ? &(*allParseResults)[i] : nullptr;
        exportParserBlock(ss, tables[i].first, tables[i].second, times[i], grammar, exPtr, prPtr);
    }
    ss << "\n  }\n";

    ss << "}";
    return ss.str();
}

