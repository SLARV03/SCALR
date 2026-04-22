#ifndef LRCOMMON_HPP
#define LRCOMMON_HPP

#include "Shared_Structs.hpp"
#include <vector>
#include <map>
#include <set>

namespace LRCommon {

// Canonical collection of LR item sets
struct CanonicalCollection {
    std::vector<ItemSet> states;
    std::map<ItemSet, int> stateIndex;
    // transitions[stateId][symbol] = targetStateId
    std::map<int, std::map<Symbol, int>> transitions;
};

// ── Closure functions ──────────────────────────────────────────
ItemSet closureLR0(const ItemSet& items, const Grammar& grammar);
ItemSet closureLR1(const ItemSet& items, const Grammar& grammar);

// ── GOTO functions ─────────────────────────────────────────────
ItemSet gotoLR0(const ItemSet& items, const Symbol& symbol, const Grammar& grammar);
ItemSet gotoLR1(const ItemSet& items, const Symbol& symbol, const Grammar& grammar);

// ── Build canonical collections ────────────────────────────────
CanonicalCollection buildLR0Collection(const Grammar& grammar);
CanonicalCollection buildLR1Collection(const Grammar& grammar);

// ── Populate SHIFT and GOTO entries from a canonical collection ─
void populateShiftsAndGotos(ParsingTable& table, const CanonicalCollection& cc, const Grammar& grammar);

// ── Helper: extract core items (strip lookahead) from an ItemSet ─
std::set<std::pair<int,int>> getCore(const ItemSet& items);

} // namespace LRCommon

#endif 
