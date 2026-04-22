#ifndef SETGENERATOR_HPP
#define SETGENERATOR_HPP

#include "Shared_Structs.hpp"

class SetGenerator {
public:
    static void computeFirst(Grammar& grammar);
    static void computeFollow(Grammar& grammar);
    
    // Get FIRST set for a sequence of symbols
    static std::set<Symbol> getFirstOfString(const std::vector<Symbol>& s, const Grammar& grammar);
};

#endif
