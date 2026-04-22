#ifndef METAPARSER_HPP
#define METAPARSER_HPP

#include "Shared_Structs.hpp"
#include <string>

class MetaParser {
public:
    static Grammar parse(const std::string& input);
    
private:
    static std::string trim(const std::string& s);
};

#endif
