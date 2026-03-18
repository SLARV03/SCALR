#ifndef SHARED_STRUCTS_HPP
#define SHARED_STRUCTS_HPP
//Everything is a struct, if u think of something, its probably a struct already. GG
#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <algorithm>
#include <functional>

using Symbol = std::string; //string to represent grammar elements

//Prod rule struct
struct Rule {
    int id;
    Symbol lhs;
    std::vector<Symbol> rhs;
    int sourceLine; // line number to monitor for error reporting

    std::string toString() const {
        std::string s = lhs + " ->";
        if (rhs.empty()) s += " epsilon";
        for (const auto& sym : rhs) s += " " + sym;
        return s;
    }

    bool operator==(const Rule& other) const {
        return id == other.id;
    }
};

// Represents an LR(0) or LR(1) item (LR0 items have empty lookahead)
// May make different item structs later (currently this is better for reusability)
struct Item {
    int ruleId;
    int dotPosition;
    std::set<Symbol> lookahead;

    bool operator<(const Item& other) const {
        if (ruleId != other.ruleId) return ruleId < other.ruleId;
        if (dotPosition != other.dotPosition) return dotPosition < other.dotPosition;
        return lookahead < other.lookahead;
    }
    
    bool operator==(const Item& other) const {
        return ruleId == other.ruleId && dotPosition == other.dotPosition && lookahead == other.lookahead;
    }

    bool isCoreEqual(const Item& other) const {
        return ruleId == other.ruleId && dotPosition == other.dotPosition;
    }
};

using ItemSet = std::set<Item>;

// Grammar
// Yes,grammar is an object as a whole, using this for segregation and additional data)
struct Grammar {
    std::vector<Rule> rules;
    std::set<Symbol> terminals;
    std::set<Symbol> nonTerminals;
    Symbol startSymbol;
    Symbol augmentedStart; //S'
    bool isAugmented = false;
    
    std::map<Symbol, std::set<Symbol>> firstSets;
    std::map<Symbol, std::set<Symbol>> followSets;

    void addRule(const Symbol& lhs, const std::vector<Symbol>& rhs, int line = 0) {
        rules.push_back({(int)rules.size(), lhs, rhs, line});
        nonTerminals.insert(lhs);
    }

    // Must be called AFTER all rules are added to correctly identify terminals
    void classifySymbols() {
        for (const auto& rule : rules) {
            for (const auto& sym : rule.rhs) {
                if (nonTerminals.find(sym) == nonTerminals.end()) {
                    terminals.insert(sym);
                }
            }
        }
    }

    // Augment grammar, creates "rule 0" of sorts
    void augment() {
        if (isAugmented || rules.empty()) return;
        augmentedStart = startSymbol + "'";
        Rule augRule;
        augRule.id = 0;
        augRule.lhs = augmentedStart;
        augRule.rhs = { startSymbol };
        augRule.sourceLine = 0;
        
        // Shift all existing rule IDs
        for (auto& r : rules) r.id++;
        rules.insert(rules.begin(), augRule);
        
        nonTerminals.insert(augmentedStart);
        isAugmented = true;
    }
};

//Actions for parsing 
enum class ActionType { SHIFT, REDUCE, ACCEPT, ERROR };

struct Action {
    ActionType type;
    int target; // State ID for SHIFT, Rule ID for REDUCE
    
    bool operator==(const Action& other) const {
        return type == other.type && target == other.target;
    }
    
    std::string toString() const {
        switch (type) {
            case ActionType::SHIFT: return "s" + std::to_string(target);
            case ActionType::REDUCE: return "r" + std::to_string(target);
            case ActionType::ACCEPT: return "acc";
            case ActionType::ERROR: return "err";
        }
        return "";
    }
};

// Parsing Table
struct ParsingTable {
    std::map<int, std::map<Symbol, std::vector<Action>>> actionTable;
    std::map<int, std::map<Symbol, int>> gotoTable;
    int numStates = 0;
    
    struct Conflict {
        std::string type; // "S/R" or "R/R"
        int state;
        Symbol symbol;
        std::vector<int> rules;
    };
    std::vector<Conflict> conflicts;

    void addAction(int state, const Symbol& sym, const Action& action) {
        auto& actions = actionTable[state][sym];
        // don't add duplicates
        for (const auto& a : actions) {
            if (a == action) return;
        }
        actions.push_back(action);
    }

    void detectConflicts() {
        conflicts.clear();
        for (auto& [stateId, row] : actionTable) {
            for (auto& [sym, actions] : row) {
                if (actions.size() > 1) {
                    Conflict conflict;
                    conflict.state = stateId;
                    conflict.symbol = sym;
                    bool hasShift = false, hasReduce = false;
                    for (const auto& act : actions) {
                        if (act.type == ActionType::SHIFT) hasShift = true;
                        if (act.type == ActionType::REDUCE) {
                            hasReduce = true;
                            conflict.rules.push_back(act.target);
                        }
                    }
                    if (hasShift && hasReduce) conflict.type = "S/R";
                    else if (hasReduce && actions.size() > 1) conflict.type = "R/R";
                    else conflict.type = "S/S"; // rare
                    conflicts.push_back(conflict);
                }
            }
        }
    }
};

// Token for lexer (line, col for tracking)
struct Token {
    std::string type;
    std::string value;
    int line;
    int col;
};

#endif
