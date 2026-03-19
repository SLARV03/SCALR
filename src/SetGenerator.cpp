#include "SetGenerator.hpp"

void SetGenerator::computeFirst(Grammar& grammar) {
    // Initialize FIRST for terminals: FIRST(a) = {a}
    for (const auto& t : grammar.terminals) {
        grammar.firstSets[t].insert(t);
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& rule : grammar.rules) {
            Symbol lhs = rule.lhs;

            if (rule.rhs.empty()) {
                if (grammar.firstSets[lhs].insert("epsilon").second)
                    changed = true;
                continue;
            }

            bool allNullable = true;
            for (const auto& sym : rule.rhs) {
                if (grammar.terminals.count(sym)) {
                    if (grammar.firstSets[lhs].insert(sym).second)
                        changed = true;
                    allNullable = false;
                    break;
                }

                // sym is a non-terminal (or unknown = treat as terminal)
                if (grammar.nonTerminals.find(sym) == grammar.nonTerminals.end()) {
                    // Unknown symbol, treat as terminal
                    if (grammar.firstSets[lhs].insert(sym).second)
                        changed = true;
                    allNullable = false;
                    break;
                }

                // sym is a known non-terminal, add FIRST(sym) - {epsilon}
                for (const auto& f : grammar.firstSets[sym]) {
                    if (f != "epsilon") {
                        if (grammar.firstSets[lhs].insert(f).second)
                            changed = true;
                    }
                }

                if (grammar.firstSets[sym].find("epsilon") == grammar.firstSets[sym].end()) {
                    allNullable = false;
                    break;
                }
            }

            if (allNullable) {
                if (grammar.firstSets[lhs].insert("epsilon").second)
                    changed = true;
            }
        }
    }
}

void SetGenerator::computeFollow(Grammar& grammar) {
    if (grammar.nonTerminals.empty()) return;

    // Place $ in FOLLOW of the augmented start symbol
    Symbol startSym = grammar.isAugmented ? grammar.augmentedStart : grammar.startSymbol;
    grammar.followSets[startSym].insert("$");
    // Also put $ in FOLLOW of original start symbol if augmented
    if (grammar.isAugmented) {
        grammar.followSets[grammar.startSymbol].insert("$");
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& rule : grammar.rules) {
            for (size_t i = 0; i < rule.rhs.size(); ++i) {
                Symbol B = rule.rhs[i];
                if (grammar.nonTerminals.find(B) == grammar.nonTerminals.end()) continue;

                // beta = symbols after B
                std::vector<Symbol> beta(rule.rhs.begin() + i + 1, rule.rhs.end());

                std::set<Symbol> firstBeta = getFirstOfString(beta, grammar);
                for (const auto& f : firstBeta) {
                    if (f != "epsilon") {
                        if (grammar.followSets[B].insert(f).second)
                            changed = true;
                    }
                }

                // If beta derives epsilon (or is empty), add FOLLOW(A) to FOLLOW(B)
                if (beta.empty() || firstBeta.count("epsilon")) {
                    for (const auto& f : grammar.followSets[rule.lhs]) {
                        if (grammar.followSets[B].insert(f).second)
                            changed = true;
                    }
                }
            }
        }
    }
}

std::set<Symbol> SetGenerator::getFirstOfString(const std::vector<Symbol>& s, const Grammar& grammar) {
    std::set<Symbol> result;
    if (s.empty()) {
        result.insert("epsilon");
        return result;
    }

    for (const auto& sym : s) {
        // Terminal or unknown symbol
        if (grammar.terminals.count(sym) || grammar.nonTerminals.find(sym) == grammar.nonTerminals.end()) {
            result.insert(sym);
            return result;
        }

        // Non-terminal
        auto it = grammar.firstSets.find(sym);
        if (it == grammar.firstSets.end()) return result; // safety

        bool nullable = false;
        for (const auto& f : it->second) {
            if (f == "epsilon") nullable = true;
            else result.insert(f);
        }
        if (!nullable) return result;
    }

    result.insert("epsilon");
    return result;
}
