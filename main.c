#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

// Constants returned as part of map mechanisms
#define MAP_OK           0
#define MAP_ERR_NULL_MAP 1
#define MAP_ERR_NULL_ELE 2
#define MAP_OPERATION_FAILED 3

/*************/
/* Utilities */
/*************/
// Prints error message and exists program
#define ERROR(msg) { \
    fprintf(stdout, "error in %s: %s", __func__ , msg); \
    exit(EXIT_FAILURE); \
}

// Returns the given value, checking that it is not null first
#define NOTNULL(val) (assert(val && "this value shouldn't be null!"), val)

// Asserts that passed element is not null
#define NULLCHECK(val) assert(val && "this value shouldn't be null!")
#define NULLCHECK2(val1, val2) NULLCHECK(val1); NULLCHECK(val2)
#define NULLCHECK3(val1, val2, val3) NULLCHECK2(val1, val2); NULLCHECK(val3)
#define NULLCHECK4(val1, val2, val3, val4) NULLCHECK3(val1, val2, val3); NULLCHECK(val4)
#define NULLCHECK5(val1, val2, val3, val4, val5) NULLCHECK4(val1, val2, val3, val4); NULLCHECK(val5)

// Function that does nothing
void do_nothing(void* _) {
    ;
}

// Printer that prints nothing
void noop_printer(FILE* out_f, const void* to_print) {
    ;
}

// Allocate and return a clone of the passed in string
char* strclone(const char* to_clone) {
    char* result = malloc(sizeof(char) * (strlen(to_clone) + 1)); // +1 is for terminator
    strcpy(result, to_clone);

    return result;
}

// Map key cloner for strings
const void* str_cloner(const void* to_clone) {
    return (const void*) strclone((const char*) to_clone);
}

// Map shallow cloneer
const void* shallow_cloner(const void* to_clone) {
    return to_clone;
}

/************************************************************************************/
/* Generic map datastructure. Implemented using a BST. Also used to represent sets. */
/************************************************************************************/
// Custom strcmp function to be used in map_t
int strcomp(const void* lhs, const void* rhs) {
    return strcmp((const char*) lhs, (const char*) rhs);
}

// Function that compares integer used in map_t
int intcmp(const void* lhs, const void* rhs) {
    return (intptr_t) lhs - (intptr_t) rhs;
}

// Custom duplication handling function that singals the failed instertion
int signal_insertion_fail(const void* key, void* old_ele, void* new_ele) {
    return MAP_OPERATION_FAILED;
}

// Custom duplication handling function that throws an error
// (needed for when duplicates should not be encountered)
int disallow_duplicates(const void* key, void* old_ele, void* new_ele) {
    ERROR("Duplicates are not allowed here.\n");
    return MAP_OPERATION_FAILED;
}

// Prints element as a string
void str_printer(FILE* out_f, const void* to_print) {
    fprintf(out_f, "%s", (const char*) to_print);
}

// Prints element as int
void int_printer(FILE* out_f, const void* to_print) {
    fprintf(out_f, "%" PRIiPTR "", (intptr_t) to_print);
}

// Function type used to customize map ordering
typedef int (*compfun_t)(const void* lhs, const void* rhs);

// Function type used to customize key copying
typedef const void* (*key_cloner_fun_t)(const void* key);

// Type for functions meant to allocate and initialize new map element
typedef void* (*map_ele_maker_fun_t)();

// Function type used to customize map duplicate handling
typedef int (*handle_dup_fun_t)(const void* key, void* old, void* new);

// Function type used to customize map entry printing
typedef void (*printer_fun_t)(FILE* out_f, const void* to_print);

// Function type used to customize element deallocation
typedef void (*free_element_fun_t) (void* to_free);

// Function type used to customize element deallocation
typedef void (*free_key_fun_t) (void* to_free);

// Node type
typedef struct _map_node_t {
    const char* key;
    void* data;
    struct _map_node_t* parent;
    struct _map_node_t* left;
    struct _map_node_t* right;
} map_node_t;

// Set type
typedef struct _map_t {
    map_node_t* root;
    int len;
    compfun_t comp;
    key_cloner_fun_t clone_key;
    handle_dup_fun_t handle_dup;
    free_element_fun_t free_element;
    free_key_fun_t free_key;
} map_t;

// Create a new node with no children
map_node_t* map_node_new(const char* key, void* data, map_node_t* parent, key_cloner_fun_t clone_key) {
    map_node_t* result = malloc(sizeof(map_node_t));
    result->parent = parent;
    result->left = NULL;
    result->right = NULL;
    result->key = clone_key(key);
    result->data = data;
    return result;
}

// Check if given node has no children
int node_is_leaf(const map_node_t* node) {
    return node && node->right && node->left;
}

// Create a new empty map
map_t* map_empty( compfun_t comp, key_cloner_fun_t clone_key,
        handle_dup_fun_t handle_dup, free_key_fun_t free_key,
        free_element_fun_t free_element) {
    map_t* result = malloc(sizeof(map_t));

    result->root = NULL;
    result->len = 0;
    result->comp = comp;
    result->clone_key = clone_key;
    result->handle_dup = handle_dup;
    result->free_key = free_key;
    result->free_element = free_element;

    return result;
}

// Various ways of freeing memory allocated by given map
void node_free(map_node_t*, free_key_fun_t, free_element_fun_t);
int map_free(map_t* map) {
    if (!map)
        return MAP_ERR_NULL_MAP;

    node_free(map->root, map->free_key, map->free_element); map->root = NULL;
    free(map);

    return MAP_OK;
}
void map_vfree(void* map) {
    int result = map_free((map_t*) map);
    if (result != MAP_OK) {
        ERROR("Encountered error in freeing map!");
    }
}
void node_free(map_node_t* node, free_key_fun_t free_key, free_element_fun_t free_ele) {
    if (node) {
        // Free data the node contains.
        free_key((void*) node->key);
        free_ele(node->data);

        // Recursively free left and right nodes
        node_free(node->left, free_key, free_ele);
        node_free(node->right, free_key, free_ele);

        // Free memory occupied by node structure itself
        free(node);
    }
}

// Print the contents of a map in order
#define PRINT_MODE_CUSTOM 0
#define PRINT_MODE_DB 1
#define PRINT_MODE_SET 2
void node_print(FILE*, map_node_t*, printer_fun_t, printer_fun_t, int);
int map_print_with(FILE* out_f, const map_t* map,
        printer_fun_t print_key,
        printer_fun_t print_ele,
        int print_mode) {
    if (!map)
        return MAP_ERR_NULL_MAP;

    if (print_mode == PRINT_MODE_DB || print_mode == PRINT_MODE_SET)
        fputs("{ ", out_f);

    node_print(out_f, map->root, print_key, print_ele, print_mode);

    if (print_mode == PRINT_MODE_DB || print_mode == PRINT_MODE_SET)
        fputs("}", out_f);

    return MAP_OK;
}
int set_print_with(FILE* out_f, const map_t* set,
        printer_fun_t print_ele,
        int print_mode) {
    return map_print_with(out_f, set, print_ele, &noop_printer, print_mode);
}
int map_print(FILE* out_f, map_t* map, int print_mode) {
    return map_print_with(out_f, map, &str_printer, &str_printer, print_mode);
}
int map_db_print(FILE* out_f, map_t* map) {
    return map_print(out_f, map, PRINT_MODE_DB);
}
// Helper function to aid recursion in map printing 
void node_print(FILE* out_f, map_node_t* node,
        printer_fun_t print_key,
        printer_fun_t print_ele,
        int print_mode) {
    if (node) {
        node_print(out_f, node->left, print_key, print_ele, print_mode);

        if (print_mode == PRINT_MODE_DB) fputs("(", out_f);
        print_key(out_f, node->key);
        if (print_mode == PRINT_MODE_DB) fputs(": ", out_f);
        print_ele(out_f, node->data);
        if (print_mode == PRINT_MODE_DB) fputs(") ", out_f);
        else if (print_mode == PRINT_MODE_SET) fputs(", ", out_f);

        node_print(out_f, node->right, print_key, print_ele, print_mode);
    }
}

// Insert element at rightmost node 
int node_add(map_node_t**, map_node_t*, compfun_t comp, handle_dup_fun_t, map_node_t*);
void node_insert_at_rightmost(map_node_t** root, map_node_t* to_insert) {
    if (root == NULL) {
        ERROR("Null pointer ref");
    }

    if (*root == NULL) {
        *root = to_insert;
        return;
    }

    node_insert_at_rightmost(&((*root)->right), to_insert);
}

// Insert element at leftmost node
void node_insert_at_leftmost(map_node_t** root, map_node_t* to_insert) {
    if (root == NULL) {
        ERROR("Null pointer ref");
    }

    if (*root == NULL) {
        *root = to_insert;
        return;
    }

    node_insert_at_leftmost(&((*root)->left), to_insert);
}


// Add an element to the map
int map_add(map_t* map, const void* key, void* element) {
    if (element == NULL)                            
        return MAP_ERR_NULL_ELE;                    
    if (map == NULL)
        return MAP_ERR_NULL_MAP;

    // TODO: OPT: make node_inplace_add
    map_node_t* node_to_add = map_node_new(key, element, NULL, map->clone_key);
    int result = node_add(&(map->root), node_to_add, map->comp, map->handle_dup, NULL);

    if (result == MAP_OPERATION_FAILED) {
        // TODO: OPT: Optimization opportunity: do not do this free
        node_free(node_to_add, map->free_key, map->free_element);
        return MAP_OPERATION_FAILED;
    }

    map->len++;
    return MAP_OK;
}                                                   
// Variant for sets
map_node_t** node_get_ref(map_node_t**, const void*, compfun_t); // forward dec.
int set_add(map_t* set, const void* element) {
    map_node_t** target_node_ref = NOTNULL(node_get_ref(&(set->root), element, set->comp));

    if (*target_node_ref == NULL)  {
        *target_node_ref = map_node_new(element, (void*) element, NULL, set->clone_key);
        (*target_node_ref)->data = (void*) (*target_node_ref)->key;

        set->len++;

        return MAP_OK;
    } else  {
        return set->handle_dup(element, (*target_node_ref)->data, (void*) element);
    }
}
// Helper function to aid recursion
//  NB. cur_parent is meant to be NULL at first call
int node_add(map_node_t** root_ref, map_node_t* to_add,
        compfun_t comp, handle_dup_fun_t handle_dup,
        map_node_t* cur_parent) {
    if (root_ref == NULL) {
        ERROR("Null pointer ref");
    }

    map_node_t* root = *root_ref;

    if (root == NULL) {
        to_add->parent = cur_parent;
        *root_ref = to_add;
        return MAP_OK;
    } else {
        int comp_res = comp(to_add->key, root->data);
        if (comp_res == 0) {
            return handle_dup(to_add->key, root->data, to_add->data);
        } else if (comp_res < 0) {
            return node_add(&(root->left), to_add, comp, handle_dup, root);
        } else {
            return node_add(&(root->right), to_add, comp, handle_dup, root);
        }
    }
}

// Retrieves the specified element from a map. Returns NULL if there is no
// such element inside the map.
map_node_t* node_get(map_node_t*, const void*, compfun_t);
void* map_get(map_t* map, const void* element) {
    // Search element starting from the map's root node using its associated comparison function.
    const map_node_t* wanted_node = node_get(map->root, element, map->comp);

    // A null node from node_get means the element was not found; return NULL in turn to indicate
    // the same thing.
    if (wanted_node == NULL)
        return NULL;

    // Element was found!
    return wanted_node->data;
}
// Helpers for node_get function.
map_node_t** node_get_ref_and_parent(map_node_t** root_ref, const void* key, compfun_t comp,
                                     map_node_t** parent_ret) {
    NULLCHECK2(root_ref, parent_ret);

    map_node_t* root = *root_ref;

    if (!root) {
        return root_ref;
    }
    
    // Navigate tree
    int comp_result = comp(key, root->key);
    if (comp_result == 0) {
        return root_ref;
    } else if (comp_result < 0) {
        *parent_ret = root;
        return node_get_ref(&(root->left), key, comp);
    } else {
        *parent_ret = root;
        return node_get_ref(&(root->right), key, comp);
    }
}
map_node_t** node_get_ref(map_node_t** root_ref, const void* element, compfun_t comp) {  
    map_node_t* _;
    return node_get_ref_and_parent(root_ref, element, comp, &_);
}
map_node_t* node_get(map_node_t* root, const void* element, compfun_t comp) {
    // TODO: Optimization chance: unnecessary ref/deref could be removed by allowing some code duplication.
    return *(node_get_ref(&root, element, comp));
}

// Attempts to retrieve a specified element, replacing it with a value produced from a
// given funptr if it [the element to retrieve] is not present
void* map_get_or(map_t* map, const void* key, map_ele_maker_fun_t make_ele) {
    map_node_t* parent;
    map_node_t** found_node_ref = NOTNULL(node_get_ref_and_parent(&(map->root), key, map->comp, &parent));

    if (!(*found_node_ref)) {
        *found_node_ref = map_node_new(key, make_ele(), parent, map->clone_key);
        map->len++;
    }

    return (*found_node_ref)->data;
}

// Gets the map entry corresponding to the key with the greatest value in the map
map_node_t* node_get_rightmost(map_node_t* root, compfun_t comp) {
    if (!root || !(root->right)) return root;

    return node_get_rightmost(root->right, comp);
}
map_node_t* map_get_max_node(const map_t* map) {
    return node_get_rightmost(map->root, map->comp);
}

// Remove an element from a given map and return it
map_node_t* node_remove(map_node_t**, compfun_t comp);
int map_remove(map_t* map, const void* ele_to_remove) {
    NULLCHECK2(map, ele_to_remove);

    map_node_t** node_to_remove = node_get_ref(&(map->root), ele_to_remove, map->comp);

    if (node_to_remove) {
        map->len--;

        node_free(
            node_remove(node_to_remove, map->comp),
            map->free_key,
            map->free_element
        );

        return MAP_OK;
    } else {
        return MAP_OPERATION_FAILED;
    }
}
// Helper function to remove element from map node
map_node_t* node_remove(map_node_t** to_remove_ref, compfun_t comp) {
    if (!to_remove_ref) {
        return NULL;
    }

    map_node_t* to_remove = *to_remove_ref;                           

    // Get correct information                                        
    // TODO: optimization opportunity: do not choose tree albitrarily 
    map_node_t* substitute = NULL;                                    
    map_node_t** overlapping_top = NULL;                              
    map_node_t** overlapping_bottom = NULL;                           
    map_node_t** to_shift = NULL;                                     
    void (*overlap_insert)(map_node_t**, map_node_t*) = NULL;         
    if (to_remove->right) {                                           
        substitute = to_remove->right;                                
        overlapping_top = &(to_remove->left);                         
        if (substitute) {                                             
            to_shift = &(substitute->right);                          
            overlapping_bottom = &(substitute->left);                 
        }                                                             
        overlap_insert = &node_insert_at_leftmost;                    
    } else {                                                          
        substitute = to_remove->left;                                 
        overlapping_top = &(to_remove->right);                        
        if (substitute) {                                             
            overlapping_bottom = &(substitute->right);                
            to_shift = &(substitute->left);                           
        }                                                             
        overlap_insert = &node_insert_at_rightmost;                   
    }                                                                 

    if (substitute) {
        // Handle overlap
        overlap_insert(overlapping_bottom, *overlapping_top);
        *overlapping_top = NULL;

        // Set substition fields
        substitute->parent = to_remove->parent;
    }

    // Do substitution
    *to_remove_ref = substitute;

    // Isolate removed node and return it
    to_remove->parent = NULL;
    to_remove->left = NULL;
    to_remove->right = NULL;
    return to_remove;
}

// Variant for map_inner_remove also collecting length for optimization purpuses
int map_inner_remove_get_len(map_t* outer_map, const void* outer_key, const void* inner_key,
                             int* len_ret) {
    NULLCHECK(len_ret);

    map_node_t** inner_map_node_ref = node_get_ref(&(outer_map->root), outer_key, outer_map->comp);
    if (*inner_map_node_ref) {
        map_t* inner_map = (map_t*) (*inner_map_node_ref)->data;
        int res = map_remove(inner_map, inner_key);

        *len_ret = inner_map->len;

        if (res == MAP_OK) {
            // Completely remove outer entry from map if inner map is emptied
            if (inner_map->len == 0) {
                map_node_t* removed = node_remove(inner_map_node_ref, outer_map->comp);
                node_free(removed, outer_map->free_key, outer_map->free_element);
            }
            return MAP_OK;
        } else {
            return res;
        }
    }
    return MAP_OPERATION_FAILED;
}

// Remove an element from a map of maps, also removing outer map element if the inner
// map is left empty
int map_inner_remove(map_t* outer_map, const void* outer_key, const void* inner_key) {
    int _;
    return map_inner_remove_get_len(outer_map, outer_key, inner_key, &_);
}



/***************************************************************/
/* String set interface built on top of BST map implementation */
/***************************************************************/
// Allocate empty string set (variation of map)
map_t* strset_empty() {
    map_t* result = malloc(sizeof(map_t));

    result->root = NULL;
    result->len = 0;
    result->comp = &strcomp;
    result->clone_key = &str_cloner;
    result->handle_dup = &disallow_duplicates;
    // Sets have identical keys and elements, so they need to be freed only once
    result->free_key = &free;
    result->free_element = &do_nothing;

    return result;
}

// Allocate string set (variation of map) with a single element inside it
map_t* strset_single(const char* element) {
    map_t* result = malloc(sizeof(map_t));

    result->root = map_node_new((const void*) element, (void*) element, NULL, &str_cloner);
    result->len = 1;
    result->comp = &strcomp;
    result->handle_dup = &disallow_duplicates;
    // Sets have identical keys and elements, so they need to be freed only once
    result->free_key = &free;
    result->free_element = &do_nothing;

    return result;
}
// Used to allocate for compatibility with map_t interface
void* v_strset_empty() {
    return (void*) strset_empty();
}

// Used to print strsets in maps
void strset_printer(FILE* out_f, const void* to_print) {
    map_print_with(out_f, (const map_t*) to_print, &str_printer, &noop_printer, PRINT_MODE_SET);
}

/**********************************************/
/* Data types used to construct relations map */
/**********************************************/
typedef struct relinfo_t_ {
    map_t* rxing_ents_map;    // map of (str: set(str))
    map_t* rxing_amounts_map; // map of (int: set(str))
} relinfo_t;
// Custom free function
void relinfo_free(void* to_free_v) {
    relinfo_t* to_free = (relinfo_t*) to_free_v;

    map_free(to_free->rxing_ents_map);
    map_free(to_free->rxing_amounts_map);

    free(to_free);
}

// Allocate new empty relinfo
relinfo_t* relinfo_empty() {
    relinfo_t* result = malloc(sizeof(relinfo_t));

    result->rxing_ents_map = map_empty(&strcomp, &str_cloner, &disallow_duplicates, &free, &map_vfree);
    result->rxing_amounts_map = map_empty(&intcmp, &shallow_cloner, &disallow_duplicates, &do_nothing, &map_vfree);

    return result;
}
void* v_relinfo_empty() {
    return (void*) relinfo_empty();
}

// 1 if relinfo is empty, 0 otherwise 
int relinfo_is_empty(const relinfo_t* relinfo) {
    return relinfo->rxing_ents_map->len == 0;
}

// Custom printer for relinfo entries in maps
void relinfo_print(FILE* out_f, const void* v_relinfo) {
    fputs("ri< ", out_f);

    relinfo_t* relinfo = (relinfo_t*) v_relinfo;

    if (!relinfo) {
        puts("NULL");
    } else {
        map_print_with(out_f, relinfo->rxing_ents_map, &str_printer, &strset_printer,
                PRINT_MODE_DB);
        fputs(", ", out_f);
        map_print_with(out_f, relinfo->rxing_amounts_map, &int_printer, &strset_printer,
                PRINT_MODE_DB);
    }

    fputs(" >", out_f);
}

/*******************************************/
/* Helper functions for handling relations */
/*******************************************/
// Add a relation to the database
// TODO: check for malformed relations (such as those among entities that do not exist)  
// TODO OPT: do not clone keys everytime (rel_id, rxing_ent and txin_end are cloned w\ strclone everytime)
void rel_add(map_t* /* of relinfo_t */ relations,
             const char* txing_ent, const char* rxing_ent, const char* rel_id) {
    relinfo_t* relinfo = map_get_or(relations, rel_id, &v_relinfo_empty);

    intptr_t curr_tx_amount = 0; 

    // Associate rx_ent to tx_ent in rxing_ents_map.
    // Map layout: rx_map = {rxing_ent, tx_set = {txing_ent}}
    {
        map_t* rx_map = NOTNULL(relinfo->rxing_ents_map);
        map_t* tx_set = map_get_or(rx_map, rxing_ent, &v_strset_empty); // TODO: OPT: 
        set_add(tx_set, txing_ent);
        curr_tx_amount = tx_set->len;
    }

    // Update tx_amounts_map with new rx_ents amount associated with inserted tx_ent.
    // Map layout: amm_map = {rx_amm, rx_set = {}};
    // where rx_amm is an int indicating the number of times the entities in the associated
    // rx_set are found at the receiving end of a relation.
    //  NB. this is just kept updated for optimization purposes
    {
        // Since this function can't fail to add a new relation, the amount of
        // tx ents that the rx ent of this relation must have increased
        assert(curr_tx_amount > 0); 

        // Remove rx_ent from previous rx_set associated with old tx amount
        map_t* amm_map = NOTNULL(relinfo->rxing_amounts_map);
        map_inner_remove(amm_map, (const void*) (curr_tx_amount - 1), (const void*) rxing_ent);

        // Place rx in rx set associated with its updated tx amount
        map_t* curr_rx_set = map_get_or(amm_map, (void*) curr_tx_amount, &v_strset_empty);
        assert(set_add(curr_rx_set, rxing_ent) == MAP_OK);
    }
}

// Deletes relation from database
// TODO: find a way to simulate currying and abstract cleanup 
//       into higher order function
void rel_del(map_t* /* of relinfo_t */ relations,
             const char* txing_ent, const char* rxing_ent, const char* rel_id) {
    // Get relinfo relative to removed relation
    map_node_t** relinfo_node_ref = node_get_ref(&(relations->root), rel_id, relations->comp);
    relinfo_t* relinfo = (relinfo_t*) ((*relinfo_node_ref)->data);

    // Remove rxing_ent and txing_ent
    // TODO OPT: see if this is better left as is or if removing it is better
    //           (removal would entail speed^/mem^ opt).
    int txs_len;
    int removal_res = map_inner_remove_get_len(relinfo->rxing_ents_map, 
            (const void*) rxing_ent, 
            (const void*) txing_ent,
            &txs_len);

    if (removal_res == MAP_OK) {
        // Update tx_ammount cache
        map_inner_remove(relinfo->rxing_amounts_map,
                (const void*) (intptr_t) (txs_len + 1),
                (const void*) rxing_ent);
        if (txs_len > 0) {
            set_add(map_get_or(
                        relinfo->rxing_amounts_map, 
                        (const void*) (intptr_t) txs_len,
                        &v_strset_empty),
                    (const void*) rxing_ent);
        }

        // Cleanup relinfo if empty
        if (relinfo_is_empty(relinfo))  {
            node_free(node_remove(relinfo_node_ref, relations->comp),
                    relations->free_key,
                    relations->free_element);
        }
    }
}

// TODO: implement this with currying (see above)
/************************************************************************************/
/* // Removes relation from database                                                */
/* void rel_del(map_t* * of relinfo_t *\/ relations,                                */
/*              const char* txing_ent, const char* rxing_ent, const char* rel_id) { */
/*     void do_it(map_t* map, const void* key) {                                    */
/*         rel_del_no_cleanup(map, txing_ent, rxing_ent, (const char*) key);        */
/*     }                                                                            */
/*                                                                                  */
/*     do_then_cleanup(&do_it, &relinfo_is_empty, &relinfo_vfree);                  */
/* }                                                                                */
/*                                                                                  */
/************************************************************************************/

/******************/
/* Report command */
/******************/
void report_string_printer(FILE* out_f, const void* str) {
    fprintf(out_f, "%s ", (const char*) str);
}

void report_relinfo_printer(FILE* out_f, const void* relinfo) {
    map_node_t* max_node =
        map_get_max_node(((relinfo_t*) relinfo)->rxing_amounts_map);

    // Empty rx sets are not allowed
    assert(max_node);

    int txs_num = (int) (intptr_t) max_node->key;
    map_t* rxs = (map_t*) max_node->data;

    set_print_with(out_f, rxs, &report_string_printer, PRINT_MODE_CUSTOM);

    fprintf(out_f, "%d; ", txs_num);
}

void report(FILE* out_f, const map_t* /* of relinfo_t */ relations) {
    if (relations->len == 0) {
        fputs("none\n", out_f);
    } else  {
        map_print_with(out_f, relations, 
                &report_string_printer, 
                &report_relinfo_printer,
                PRINT_MODE_CUSTOM);
        fputs("\n", out_f);
    }
}

/**********************************************/
/* Helper functions for handling program flow */
/**********************************************/
// Config constants
#define DEBUG_ON  0
#define DEBUG_OFF 1

// Entity config constants
#define ENT_NAME_BUF_LEN 100
#define REL_NAME_BUF_LEN 100

// Configure progam
void configure(int argc, char** argv, FILE** in_f, FILE** out_f, int* debug_mode) {
    // Set input file
    if (argc > 1) *in_f = fopen(argv[1], "r");
    else *in_f = stdin;

    // Set output file
    if (argc > 2) *out_f = fopen(argv[2], "w");
    else *out_f = stdout;

    // Set debug mode
    if (argc > 3 && strcmp(argv[3], "db") == 0) *debug_mode = DEBUG_OFF;
    else *debug_mode = DEBUG_ON;
}

// Initialize empty entity storage
map_t* initialize_entities() {
    return strset_empty();
}

// Initialize empty relations storage
map_t* initialize_relations() {
    return map_empty(&strcomp, &str_cloner, &disallow_duplicates, &free, &relinfo_free);
}

/********/
/* Main */
/********/
int main(int argc, char** argv) {
    // Configure program
    FILE* in_f;
    FILE* out_f;
    int debug_mode;
    configure(argc, argv, &in_f, &out_f, &debug_mode);

    // Initialize apinet storage
    map_t* entities = initialize_entities();
    map_t* relations = initialize_relations();

    // User interaction loop
    char command[1024];
    while (1) {
        // Read head of command from user
        fscanf(in_f, "%s", command);

        // Process head of command
        if (strcmp(command, "addent") == 0) {
            // Allocate space for entity name
            char to_add[ENT_NAME_BUF_LEN];

            // Parse second command argument as entity name
            fscanf(in_f, "%s", to_add);

            // Add entity to map
            set_add(entities, (const void*) to_add);

        } else if (strcmp(command, "delent") == 0) {
            // Get name of entity to remove from first command argument
            char to_remove[ENT_NAME_BUF_LEN];
            fscanf(in_f, "%s", to_remove);

            // Perform removal
            map_remove(entities, (void*) to_remove);

        } else if (strcmp(command, "addrel") == 0) {
            // Get name of txing entity
            char txing_ent[ENT_NAME_BUF_LEN];
            fscanf(in_f, "%s", txing_ent);

            // Get name of rxing entity
            char rxing_ent[ENT_NAME_BUF_LEN];
            fscanf(in_f, "%s", rxing_ent);

            // Get name of relation
            char relation[REL_NAME_BUF_LEN];
            fscanf(in_f, "%s", relation);

            // Add relation
            rel_add(relations, txing_ent, rxing_ent, relation);

        } else if (strcmp(command, "delrel") == 0) {
            // Get name of txing entity
            char txing_ent[ENT_NAME_BUF_LEN];
            fscanf(in_f, "%s", txing_ent);

            // Get name of rxing entity
            char rxing_ent[ENT_NAME_BUF_LEN];
            fscanf(in_f, "%s", rxing_ent);

            // Get name of relation
            char relation[REL_NAME_BUF_LEN];
            fscanf(in_f, "%s", relation);

            // Add relation
            rel_del(relations, txing_ent, rxing_ent, relation);

        } else if (strcmp(command, "delent") == 0) {
            ERROR("WIP");

        } else if (strcmp(command, "report") == 0) {
            report(out_f, relations);

        // Debug mode only commands
        } else if (debug_mode == DEBUG_ON) {
            if (strcmp(command, "gent") == 0) {
                // Retrieve string to get
                char to_get[ENT_NAME_BUF_LEN];
                fscanf(in_f, "%s", to_get);

                // Get it
                void* result = map_get(entities, (void*) to_get);

                // Print the result as a string if present
                if (result == NULL)
                    puts("NOT PRESENT!");
                else 
                    puts((const char*) result);

            } else if (strcmp(command, "pent") == 0) {
                map_db_print(out_f, entities);
                fprintf(out_f, "\n");

            } else if (strcmp(command, "prel") == 0) {
                map_print_with(out_f, relations, &str_printer,
                        &relinfo_print, PRINT_MODE_DB);
                fprintf(out_f, "\n");

            } else if (strcmp(command, "quit") == 0) {
                break;

            } else {
                goto invalid_command;
            }
        } else {
invalid_command: {
                     printf("Unrecognized command: %s", command);
                     exit(EXIT_FAILURE);
                 }
        }

    }

    // Deallocate entity map
    map_free(entities);
    map_free(relations);

    // Close streams if necessary
    if (in_f != stdin)   fclose(in_f);
    if (out_f != stdout) fclose(out_f);

    // Exit
    return 0;
}
