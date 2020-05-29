#include "symbol.h"

SymbolTable::SymbolTable(SymbolTable *parent) : parent(parent) {}

void SymbolTable::ClearAll() {
    symbols.clear();
    classTypes.clear();
    enumTypes.clear();
}