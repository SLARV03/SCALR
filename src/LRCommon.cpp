#include "LRCommon.hpp"
#include "SetGenerator.hpp"
#include <algorithm>

namespace LRCommon {

// LR(0) Closure: No lookahead
ItemSet closureLR0(const ItemSet& items, const Grammar& grammar) {
    ItemSet result = items;
    bool changed = true;
    while (changed) {
        changed = false;
        ItemSet snapshot = result;
        for (const auto& item : snapshot) {
            const Rule& rule = grammar.rules[item.ruleId];
            if (item.dotPosition >= (int)rule.rhs.size()) continue;
            
            Symbol B = rule.rhs[item.dotPosition];
            if (!grammar.nonTerminals.count(B)) continue;
            
            for (const auto& r : grammar.rules) {
                if (r.lhs != B) continue;
                Item newItem{r.id, 0, {}};
                if (!result.count(newItem)) {
                    result.insert(newItem);
                    changed = true;
                }
            }
        }
    }
    return result;
}

// LR(1) Closure: With lookahead propagation
ItemSet closureLR1(const ItemSet& items, const Grammar& grammar) {
    ItemSet result = items;
    bool changed = true;
    
    while (changed) {
        changed = false;
        ItemSet snapshot = result;
        for (const auto& item : snapshot) {
            const Rule& rule = grammar.rules[item.ruleId];
            if (item.dotPosition >= (int)rule.rhs.size()) continue;
            
            Symbol B = rule.rhs[item.dotPosition];
            if (!grammar.nonTerminals.count(B)) continue;
            
            // Compute FIRST(beta a) where A -> alpha . B beta, lookahead = {a}
            std::vector<Symbol> beta(rule.rhs.begin() + item.dotPosition + 1, rule.rhs.end());
            
            for (const auto& a : item.lookahead) {
                // Compute FIRST(beta a)
                std::vector<Symbol> betaA = beta;
                betaA.push_back(a);
                std::set<Symbol> firstBetaA = SetGenerator::getFirstOfString(betaA, grammar);
                firstBetaA.erase("epsilon");
                
                for (const auto& r : grammar.rules) {
                    if (r.lhs != B) continue;
                    // For each lookahead b in FIRST(beta a), add [B -> . gamma, b]
                    for (const auto& b : firstBetaA) {
                        Item newItem{r.id, 0, {b}};
                        // Check if this exact item exists
                        auto it = result.find(newItem);
                        if (it == result.end()) {
                            result.insert(newItem);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
    return result;
}

// GOTO functions
ItemSet gotoLR0(const ItemSet& items, const Symbol& symbol, const Grammar& grammar) {
    ItemSet kernel;
    for (const auto& item : items) {
        const Rule& rule = grammar.rules[item.ruleId];
        if (item.dotPosition < (int)rule.rhs.size() && rule.rhs[item.dotPosition] == symbol) {
            Item next{item.ruleId, item.dotPosition + 1, {}};
            kernel.insert(next);
        }
    }
    return closureLR0(kernel, grammar);
}

ItemSet gotoLR1(const ItemSet& items, const Symbol& symbol, const Grammar& grammar) {
    ItemSet kernel;
    for (const auto& item : items) {
        const Rule& rule = grammar.rules[item.ruleId];
        if (item.dotPosition < (int)rule.rhs.size() && rule.rhs[item.dotPosition] == symbol) {
            Item next{item.ruleId, item.dotPosition + 1, item.lookahead};
            kernel.insert(next);
        }
    }
    return closureLR1(kernel, grammar);
}


// Build Canonical Collection for LR(0)/SLR(1)
CanonicalCollection buildLR0Collection(const Grammar& grammar) {
    CanonicalCollection cc;
    
    // Initial item: S' -> . S
    Item startItem{0, 0, {}};
    ItemSet I0 = closureLR0({startItem}, grammar);
    
    cc.states.push_back(I0);
    cc.stateIndex[I0] = 0;
    
    bool changed = true;
    while (changed) {
        changed = false;
        size_t sz = cc.states.size();
        for (size_t i = 0; i < sz; ++i) {
            // Collect all symbols after dot in this state
            std::set<Symbol> symbols;
            for (const auto& item : cc.states[i]) {
                const Rule& rule = grammar.rules[item.ruleId];
                if (item.dotPosition < (int)rule.rhs.size()) {
                    symbols.insert(rule.rhs[item.dotPosition]);
                }
            }
            
            for (const auto& sym : symbols) {
                ItemSet next = gotoLR0(cc.states[i], sym, grammar);
                if (next.empty()) continue;
                
                if (!cc.stateIndex.count(next)) {
                    cc.stateIndex[next] = cc.states.size();
                    cc.states.push_back(next);
                    changed = true;
                }
                cc.transitions[i][sym] = cc.stateIndex[next];
            }
        }
    }
    return cc;
}

// Build Canonical Collection for CLR(1)
CanonicalCollection buildLR1Collection(const Grammar& grammar) {
    CanonicalCollection cc;
    
    // Initial item: [S' -> . S, $]
    Item startItem{0, 0, {"$"}};
    ItemSet I0 = closureLR1({startItem}, grammar);
    
    cc.states.push_back(I0);
    cc.stateIndex[I0] = 0;
    
    bool changed = true;
    while (changed) {
        changed = false;
        size_t sz = cc.states.size();
        for (size_t i = 0; i < sz; ++i) {
            std::set<Symbol> symbols;
            for (const auto& item : cc.states[i]) {
                const Rule& rule = grammar.rules[item.ruleId];
                if (item.dotPosition < (int)rule.rhs.size()) {
                    symbols.insert(rule.rhs[item.dotPosition]);
                }
            }
            
            for (const auto& sym : symbols) {
                ItemSet next = gotoLR1(cc.states[i], sym, grammar);
                if (next.empty()) continue;
                
                if (!cc.stateIndex.count(next)) {
                    cc.stateIndex[next] = cc.states.size();
                    cc.states.push_back(next);
                    changed = true;
                }
                cc.transitions[i][sym] = cc.stateIndex[next];
            }
        }
    }
    return cc;
}

// Populate SHIFT and GOTO entries from canonical collection
void populateShiftsAndGotos(ParsingTable& table, const CanonicalCollection& cc, const Grammar& grammar) {
    table.numStates = cc.states.size();
    for (const auto& [stateId, trans] : cc.transitions) {
        for (const auto& [sym, targetState] : trans) {
            if (grammar.terminals.count(sym)) {
                table.addAction(stateId, sym, {ActionType::SHIFT, targetState});
            } else if (grammar.nonTerminals.count(sym) && sym != grammar.augmentedStart) {
                table.gotoTable[stateId][sym] = targetState;
            }
        }
    }
}

// Helper: extract core items (strip lookahead)
std::set<std::pair<int,int>> getCore(const ItemSet& items) {
    std::set<std::pair<int,int>> core;
    for (const auto& item : items) {
        core.insert({item.ruleId, item.dotPosition});
    }
    return core;
}

} // namespace LRCommon
