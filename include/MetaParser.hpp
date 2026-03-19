#ifndef METAPARSER_HPP
#define METAPARSER_HPP

#include "Shared_Structs.hpp"
#include <string>

class MetaParser {
public:
    // Parses a raw grammar string into a Grammar object.
    // Automatically augments the grammar and classifies symbols.
    static Grammar parse(const std::string& input);
    
private:
    static std::string trim(const std::string& s);
};

#endif // METAPARSER_HPP
