#ifndef __SYM_H__
#define __SYM_H__

#include <bitset>
#include <vector>
#include <unordered_map>

using namespace std;

enum class attr {
VOID, INT, NULLX, STRING, STRUCT, 
ARRAY, FUNCTION, VARIABLE, FIELD,
TYPEID, PARAM, LOCAL, LVAL, CONST, 
VREG, VADDR, BITSET_SIZE,
};

using attr_bitset = 
std::bitset<static_cast<size_t>(attr::BITSET_SIZE)>;

// Each node in the symbol table must have information
// associated with the identifier. It will be simpler to make
// nodes in all of the symbol tables identical, and null out
// unnecessary fields.
struct symbol {
    // Symbol attributes, as described earlier.
    attr_bitset attributes;

    // For parameters and local variables, their declaration
    // sequence number, starting from 0, 1, etc. For all global
    // names, 0.
    size_t sequence;

    // A pointer to the field table if this symbol is a struct (if
    // the typeid attribute is present). Else null.
    unordered_map<const string*,symbol*>* fields;

    // A symbol node pointer which points at the parameter
    // list. For a function, points at a vector of parameters.
    // Else null.
    vector<symbol*>* parameters;
};

using symbol_table = unordered_map<const string*,symbol*>;
using symbol_entry = symbol_table::value_type;

#endif