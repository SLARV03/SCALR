#include "LALR1.hpp"
#include "LRCommon.hpp"

namespace LALR1 {

ParsingTable generate(Grammar& grammar) {
    // CLR(1) canonical collection
    LRCommon::CanonicalCollection clr = LRCommon::buildLR1Collection(grammar);
    
    // merge states with same core
    // Map from core=merged state index
    std::map<std::set<std::pair<int,int>>, int> coreToMerged;

    // convert clr state to lalr state 
    std::vector<int> clrToLalr(clr.states.size(), -1);
    
    // Merged states: item sets with merged lookahaeds 
    std::vector<ItemSet> mergedStates;
    
    for (size_t i = 0; i < clr.states.size(); ++i)
     {
        auto core = LRCommon::getCore(clr.states[i]);//checks ruleid and dot position ignore lookaheads 
        if (coreToMerged.count(core)) //  if same core  
        {  
            int mergedIdx = coreToMerged[core];
            clrToLalr[i] = mergedIdx;
            
            // Merge lookaheads
            ItemSet& existing = mergedStates[mergedIdx];
            
            // Build a map from (ruleId, dotPos) -> lookaheads
            std::map<std::pair<int,int>, std::set<Symbol>> laMap;
            for (const auto& it : existing) {
                laMap[{it.ruleId, it.dotPosition}].insert(it.lookahead.begin(), it.lookahead.end());
            }
            for (const auto& it : clr.states[i]) {
                laMap[{it.ruleId, it.dotPosition}].insert(it.lookahead.begin(), it.lookahead.end());//looahaed uniion 
            }
            
            existing.clear();//remove old states

            for (const auto& [key, la] : laMap) {
                Item item{key.first, key.second, la};
                existing.insert(item);
            }
        } else //create new state 
        {
            int newIdx = mergedStates.size();
            coreToMerged[core] = newIdx;
            clrToLalr[i] = newIdx;
            mergedStates.push_back(clr.states[i]);
        }
    }
    
    // Build LALR table from merged states
    ParsingTable table;
    table.numStates = mergedStates.size();
    
    // rebuild transitions using merged states indices 
    for (const auto& [fromClr, trans] : clr.transitions) {
        int fromLalr = clrToLalr[fromClr];
        for (const auto& [sym, toClr] : trans) {
            int toLalr = clrToLalr[toClr];
            if (grammar.terminals.count(sym)) {
                table.addAction(fromLalr, sym, {ActionType::SHIFT, toLalr});
            } else if (grammar.nonTerminals.count(sym) && sym != grammar.augmentedStart) {
                table.gotoTable[fromLalr][sym] = toLalr;
            }
        }
    }
    
    // reductions from merged states
    for (size_t i = 0; i < mergedStates.size(); ++i) {
        for (const auto& item : mergedStates[i]) {
            const Rule& rule = grammar.rules[item.ruleId];
            if (item.dotPosition != (int)rule.rhs.size()) continue;
            
            if (rule.lhs == grammar.augmentedStart) {
                table.addAction(i, "$", {ActionType::ACCEPT, 0});
            } else {
                for (const auto& la : item.lookahead) {
                    table.addAction(i, la, {ActionType::REDUCE, item.ruleId});
                }
            }
        }
    }
    
    table.detectConflicts();
    return table;
}

} // namespace LALR1
