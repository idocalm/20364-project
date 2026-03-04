#ifndef SYMTAB_H
#define SYMTAB_H

#include "ast.h"
#include "defs.h"

#define DEFAULT_BUCKET_COUNT 256

// linked list of symbols: each symbol contains a name and the type (int / float)
typedef struct symbol_entry {
    char *name;
    AstType type;
    struct symbol_entry *next;
} SymbolEntry;

// hash table for symbols
typedef struct symbol_table {
    SymbolEntry **buckets;
    u32 bucket_count;
    u32 size;
} SymbolTable;

// create a new symbol hash table
SymbolTable *new_symtable();

// free the table
void free_symtable(SymbolTable *table);

// insert a symbol to the table, return 1 if success or 0
b8 symtable_insert(SymbolTable *table, const char *name, AstType type);

// search a symbol by name
SymbolEntry *symtable_find(const SymbolTable *table, const char *name);

#endif
