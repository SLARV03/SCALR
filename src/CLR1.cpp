#include "CLR1.hpp"
#include "LRCommon.hpp"

namespace CLR1 {

ParsingTable generate(Grammar& grammar) {
    LRCommon::CanonicalCollection cc = LRCommon::buildLR1Collection(grammar);
    ParsingTable table;
    LRCommon::populateShiftsAndGotos(table, cc, grammar);
    
    // Reductions: only on the specific lookahead symbol
    for (size_t i = 0; i < cc.states.size(); ++i) {
        for (const auto& item : cc.states[i]) {
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

} // namespace CLR1
