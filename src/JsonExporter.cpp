#include "JsonExporter.hpp"
#include <sstream>
#include <chrono>

std::string JsonExporter::exportToJSON(const ParsingTable& table, const Grammar& grammar, double timeMs) {
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
    ss << "  ],\n";

    // Parse Tree (placeholder - would need actual parsing to build)
    ss << "  \"parse_tree\": null\n";
    
    ss << "}";
    return ss.str();
}
