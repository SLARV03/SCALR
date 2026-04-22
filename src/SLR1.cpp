#include "SLR1.hpp"
#include "LRCommon.hpp"

namespace SLR1 {

ParsingTable generate(Grammar& grammar) {
    LRCommon::CanonicalCollection cc = LRCommon::buildLR0Collection(grammar);
    ParsingTable table;
    LRCommon::populateShiftsAndGotos(table, cc, grammar);
    
    // Reductions: only on symbols in FOLLOW(A) for rule A -> alpha .
    for (size_t i = 0; i < cc.states.size(); ++i) {
        for (const auto& item : cc.states[i]) {
            const Rule& rule = grammar.rules[item.ruleId];
            if (item.dotPosition != (int)rule.rhs.size()) continue;
            
            if (rule.lhs == grammar.augmentedStart) {
                table.addAction(i, "$", {ActionType::ACCEPT, 0});
            } else {
                const auto& follow = grammar.followSets[rule.lhs];
                for (const auto& t : follow) {
                    table.addAction(i, t, {ActionType::REDUCE, item.ruleId});
                }
            }
        }
    }
    
    table.detectConflicts();
    return table;
}

}
