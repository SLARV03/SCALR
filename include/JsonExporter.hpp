#ifndef JSONEXPORTER_HPP
#define JSONEXPORTER_HPP

#include "Shared_Structs.hpp"
#include <string>

class JsonExporter {
public:
    static std::string exportToJSON(const ParsingTable& table, const Grammar& grammar, double timeMs);
};

#endif
