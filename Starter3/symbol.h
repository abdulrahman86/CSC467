#ifndef _SYMBOL_H
#define _SYMBOL_H

#include <stddef.h>

/**
   Singleton Object Symbol Table
 */

#define BASE_ENTRY_NUM 256
// how many entry space assigned initially for each st
#define BASE_ST_SIZE BASE_ENTRY_NUM * sizeof(struct _st_entry)

struct _st_entry;

struct _st {
    size_t depth;
    size_t entry_num;
    size_t max_entry;
    struct _st* parent_scope;
    struct _st_entry* head;
    struct _st_entry*
        tail;  // tail always points to the last element in linked list
};

struct _st_entry {
    // Public:
    int is_const;
    int has_init;
    int type_code;
    int vec_size;
    char var_name[32];
    // Private:
    struct _st_entry* _next;
    struct _st* _owner;
};

typedef struct _st_entry st_entry;
typedef struct _st symbol_table;

/**
   Methods of symbol table
   scope_enter: enter a new scope
   scope_leave: destroy all variables associated with current scope
   scope_depth: get the depth of current scope. root scope returns 0
   scope_declare_symbol: declare a symbol in current scope
   scope_find_entry: find the symbol in *previous* scopes; return NULL
                     when symbol is not found
 */

void scope_enter();
void scope_leave();
size_t scope_depth();
int scope_declare_symbol(const char* name, int is_const, int type_code,
                         int vec_size);
int scope_define_symbol(const char* name, int is_const, int type_code,
                        int vec_size);
void set_inited(st_entry* ste);  // set the has_init field to true
st_entry* scope_find_entry(const char* name);

#endif