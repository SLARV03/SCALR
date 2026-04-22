#include "ExampleGenerator.hpp"
#include <map>
#include <queue>
#include <set>
#include <algorithm>

// Computes the shortest terminal derivation for each symbol
static void computeShortestDerivations(const Grammar& grammar, std::map<Symbol, std::vector<Symbol>>& shortestDeriv) {
    std::map<Symbol, int> dist;
    
    for (const auto& t : grammar.terminals) {
        dist[t] = 1;
        shortestDeriv[t] = {t};
    }
    dist["epsilon"] = 0;
    shortestDeriv["epsilon"] = {};
    dist["ε"] = 0;
    shortestDeriv["ε"] = {};

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& rule : grammar.rules) {
            int len = 0;
            bool possible = true;
            std::vector<Symbol> expansion;
            
            for (const auto& sym : rule.rhs) {
                if (sym == "epsilon" || sym == "ε") continue;
                if (!dist.count(sym)) {
                    possible = false;
                    break;
                }
                len += dist[sym];
                const auto& sub = shortestDeriv[sym];
                expansion.insert(expansion.end(), sub.begin(), sub.end());
            }
            
            if (possible) {
                if (!dist.count(rule.lhs) || len < dist[rule.lhs]) {
                    dist[rule.lhs] = len;
                    shortestDeriv[rule.lhs] = expansion;
                    changed = true;
                }
            }
        }
    }
}

static std::vector<Symbol> findPathToState(int targetState, const ParsingTable& table) {
    if (targetState == 0) return {};
    
    std::vector<int> q;
    std::map<int, int> parent;
    std::map<int, Symbol> parentSym;
    
    q.push_back(0);
    parent[0] = -1;
    
    int head = 0;
    while(head < q.size()) {
        int u = q[head++];
        if (u == targetState) break;
        
        if (table.actionTable.count(u)) {
            for (const auto& [sym, actions] : table.actionTable.at(u)) {
                for (const auto& act : actions) {
                    if (act.type == ActionType::SHIFT) {
                        int v = act.target;
                        if (!parent.count(v)) {
                            parent[v] = u;
                            parentSym[v] = sym;
                            q.push_back(v);
                        }
                    }
                }
            }
        }
        if (table.gotoTable.count(u)) {
            for (const auto& [sym, v] : table.gotoTable.at(u)) {
                if (!parent.count(v)) {
                    parent[v] = u;
                    parentSym[v] = sym;
                    q.push_back(v);
                }
            }
        }
    }
    
    std::vector<Symbol> path;
    int curr = targetState;
    while(curr != 0 && curr != -1) {
        path.push_back(parentSym[curr]);
        curr = parent[curr];
    }
    std::reverse(path.begin(), path.end());
    return path;
}

static std::string joinSymbols(const std::vector<Symbol>& syms) {
    std::string s;
    for (size_t i = 0; i < syms.size(); ++i) {
        if (i > 0) s += " ";
        s += syms[i];
    }
    return s;
}

std::vector<ExampleString> ExampleGenerator::generate(const ParsingTable& table, const Grammar& grammar) {
    std::vector<ExampleString> examples;
    std::map<Symbol, std::vector<Symbol>> shortestDeriv;
    computeShortestDerivations(grammar, shortestDeriv);

    if (!table.conflicts.empty()) {
        // Conflict mode: generate an example for each conflict
        std::set<std::string> seen;

        for (const auto& c : table.conflicts) {
            std::vector<Symbol> path = findPathToState(c.state, table);
            // Replace non-terminals with shortest terminal derivation
            std::vector<Symbol> concreteInput;
            for (const auto& pSym : path) {
                if (shortestDeriv.count(pSym)) {
                    const auto& expansion = shortestDeriv[pSym];
                    concreteInput.insert(concreteInput.end(), expansion.begin(), expansion.end());
                } else {
                    concreteInput.push_back(pSym);
                }
            }
            
            // Append the conflict lookahead symbol (if it's not $)
            if (c.symbol != "$") {
                concreteInput.push_back(c.symbol);
            }
            
            std::string exStr = joinSymbols(concreteInput);
            if (!seen.count(exStr)) {
                seen.insert(exStr);
                ExampleString ex;
                ex.string = exStr.empty() ? "(empty string)" : exStr;
                ex.type = "conflict";
                ex.description = c.type + " conflict at state " + std::to_string(c.state) + " on symbol '" + c.symbol + "'";
                examples.push_back(ex);
            }
        }

    } else {
        // Normal mode: generate varied examples using BFS
        Symbol start = grammar.startSymbol;
        std::set<std::string> seenExamples;
        
        if (shortestDeriv.count(start)) {
            ExampleString ex;
            ex.string = joinSymbols(shortestDeriv[start]);
            if (ex.string.empty()) ex.string = "(empty string)";
            ex.type = "normal";
            ex.description = "Shortest valid derivation";
            examples.push_back(ex);
            seenExamples.insert(ex.string);
        }
        
        struct BFSNode {
            std::vector<Symbol> seq;
            int depth;
        };
        std::queue<BFSNode> q;
        q.push({ {start}, 0 });
        
        int expansions = 0;
        int maxExpansions = 500; // safety limit
        
        while(!q.empty() && examples.size() < 10 && expansions < maxExpansions) {
            BFSNode curr = q.front();
            q.pop();
            expansions++;
            
            // Find first non-terminal
            int ntIdx = -1;
            for(size_t i = 0; i < curr.seq.size(); ++i) {
                if(grammar.nonTerminals.count(curr.seq[i])) {
                    ntIdx = (int)i;
                    break;
                }
            }
            
            if (ntIdx == -1) {
                // All terminals
                std::string s = joinSymbols(curr.seq);
                if (s.empty()) s = "(empty string)";
                if (!seenExamples.count(s)) {
                    seenExamples.insert(s);
                    ExampleString ex;
                    ex.string = s;
                    ex.type = "normal";
                    ex.description = "Valid derivation (depth " + std::to_string(curr.depth) + ")";
                    examples.push_back(ex);
                }
                continue;
            }
            
            // If depth too big, force resolve using shortestDeriv
            if (curr.depth >= 6) {
                std::vector<Symbol> forced;
                bool possible = true;
                for (const auto& sym : curr.seq) {
                    if (grammar.terminals.count(sym)) {
                        forced.push_back(sym);
                    } else {
                        if (shortestDeriv.count(sym)) {
                            for(const auto& e : shortestDeriv[sym]) {
                                if (e != "epsilon" && e != "ε") forced.push_back(e);
                            }
                        } else {
                            possible = false; break;
                        }
                    }
                }
                if (possible) {
                    std::string s = joinSymbols(forced);
                    if (s.empty()) s = "(empty string)";
                    if (!seenExamples.count(s)) {
                        seenExamples.insert(s);
                        ExampleString ex;
                        ex.string = s;
                        ex.type = "normal";
                        ex.description = "Valid derivation (forced resolve)";
                        examples.push_back(ex);
                    }
                }
                continue;
            }
            
            Symbol ntToExpand = curr.seq[ntIdx];
            for (const auto& rule : grammar.rules) {
                if (rule.lhs == ntToExpand) {
                    std::vector<Symbol> nextSeq;
                    for(int i = 0; i < ntIdx; ++i) nextSeq.push_back(curr.seq[i]);
                    for(const auto& s : rule.rhs) {
                        if (s != "epsilon" && s != "ε") nextSeq.push_back(s);
                    }
                    for(size_t i = ntIdx + 1; i < curr.seq.size(); ++i) nextSeq.push_back(curr.seq[i]);
                    
                    q.push({nextSeq, curr.depth + 1});
                }
            }
        }
    }
    return examples;
}
