#ifndef VARIABLE_H
#define VARIABLE_H

#include <vector>
#include <string>

namespace scdl {

enum VariableType {
    VAR_UINT,
    VAR_INT,
    VAR_BOOL,
    VAR_BITSTRING
};

struct Variable {
    VariableType type;
    std::string name;
    std::vector<std::string> components;
};

}

#endif // VARIABLE_H
