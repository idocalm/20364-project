#include "symbols.h"

#include <stdlib.h>
#include <string.h>

/*
*   The symbol table is a hash table, which means we have a certain amount of buckets. 
*   A bucket holds a linked list of SymbolEntry elements
*   We use a hashing algorithm to hash a string into a number. Using that number,
*   we calculate the bucket_index = hash % bucket_count, and insert it to that bucket
*   
*   For finding a symbol, the process is basically the same: calculate the hash, then the bucket
*   then walk that linked list to find a match
* */


static u32 symtable_hash(const char *text) {
    // this hashing algorithm is called FNV-1a
    // credit: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
    u32 hash = 2166136261u;
    const u8 *it = (const u8 *)text;
    while (*it != 0) {
        hash ^= (u32)(*it);
        hash *= 16777619u;
        it++;
    }
    return hash;
}

static char *symtable_dup(const char *text) {
    size_t len = strlen(text);
    char *copy = (char *) malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, text, len + 1);
    return copy;
}

SymbolTable *new_symtable()
{
    SymbolTable *table = NULL;

    table = (SymbolTable *)calloc(1, sizeof(SymbolTable));
    if (table == NULL) {
        return NULL;
    }

    table->buckets = (SymbolEntry **)calloc(DEFAULT_BUCKET_COUNT, sizeof(SymbolEntry *));
    if (table->buckets == NULL) {
        free(table);
        return NULL;
    }

    table->bucket_count = DEFAULT_BUCKET_COUNT;
    table->size = 0;
    return table;
}

void free_symtable(SymbolTable *table)
{
    u32 i = 0;
    if (table == NULL) {
        return;
    }

    for (i = 0; i < table->bucket_count; i++) {
        SymbolEntry *it = table->buckets[i];
        while (it != NULL) {
            SymbolEntry *next = it->next;
            free(it->name);
            free(it);
            it = next;
        }
    }

    free(table->buckets);
    free(table);
}

SymbolEntry *symtable_find(const SymbolTable *table, const char *name) {
    u32 idx = 0;
    SymbolEntry *it = NULL;

    if (table == NULL || name == NULL || table->bucket_count == 0) {
        return NULL;
    }

    idx = symtable_hash(name) % table->bucket_count;
    it = table->buckets[idx];

    while (it != NULL) {
        if (strcmp(it->name, name) == 0) {
            // found match in the bucket
            return it;
        }
        it = it->next;
    }
    
    return NULL;
}

b8 symtable_insert(SymbolTable *table, const char *name, AstType type) {
    u32 idx = 0;
    SymbolEntry *entry = NULL;

    if (table == NULL || name == NULL || table->bucket_count == 0) {
        return false;
    }

    if (symtable_find(table, name) != NULL) {
        // already present
        return false;
    }

    idx = symtable_hash(name) % table->bucket_count;
    entry = (SymbolEntry *)calloc(1, sizeof(SymbolEntry));
    if (entry == NULL) {
        return false;
    }

    // we duplicate the name to another string so we won't be affected by any changes later on
    entry->name = symtable_dup(name);
    if (entry->name == NULL) {
        free(entry);
        return false;
    }

    entry->type = type;
    entry->next = table->buckets[idx];
    table->buckets[idx] = entry;
    table->size++;
    return true;
}
