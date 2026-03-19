#include "LR0.hpp"
#include "LRCommon.hpp"

namespace LR0 {

//builds action and goto table
ParsingTable generate(Grammar& grammar) {
    LRCommon::CanonicalCollection cc = LRCommon::buildLR0Collection(grammar);
    ParsingTable table;
    LRCommon::populateShiftsAndGotos(table, cc, grammar);
    
    // Reductions: for LR(0), reduce on ALL terminals + $
    for (size_t i = 0; i < cc.states.size(); ++i) {
        for (const auto& item : cc.states[i]) {
            const Rule& rule = grammar.rules[item.ruleId];

            //check if item is complete
            if (item.dotPosition != (int)rule.rhs.size()) continue;
            
            //accept
            if (rule.lhs == grammar.augmentedStart) {
                table.addAction(i, "$", {ActionType::ACCEPT, 0});
            } 
            
            //reduce all terminals + $
            else {
                for (const auto& t : grammar.terminals) {
                    table.addAction(i, t, {ActionType::REDUCE, item.ruleId});
                }
                table.addAction(i, "$", {ActionType::REDUCE, item.ruleId});
            }
        }
    }
    
    table.detectConflicts();
    return table;
}

} // namespace LR0
