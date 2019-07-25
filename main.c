#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Constants returned as part of set mechanisms
#define SET_OK           0
#define SET_ERR_NULL_SET 1
#define SET_ERR_NULL_ELE 2
#define SET_INSERTION_FAILED 3

/*************/
/* Utilities */
/*************/

// Prints error message and exists program
#define ERROR(msg) { \
    fprintf(stdout, "error in %s: %s", __func__ , msg); \
    exit(EXIT_FAILURE); \
}


/************************************************************************************/
/* Generic set datastructure. Implemented using a BST. Also used to represent sets. */
/************************************************************************************/
// Custom, non type-safe, strcmp function that works nicely with the set_t interface
int strcomp(const void* lhs, const void* rhs) {
    return strcmp((const char*) lhs, (const char*) rhs);
}

// Custom duplication handling function for strings set that simply discards the newly inserted o
int discard_dup(void* old_ele, void* new_ele) {
    return SET_INSERTION_FAILED;
}

// Custom duplication handling function that throws an error
// (needed for when duplicates should not be encountered)
int disallow_duplicates(void* old_ele, void* new_ele) {
    ERROR("Duplicates should are not allowed here.\n");
    return SET_INSERTION_FAILED;
}

// Function type used to customize set ordering
typedef int (*compfun_t)(const void* lhs, const void* rhs);

// Function type used to customize set duplicate handling
typedef int (*handle_dup_fun_t)(void* old, void* new);

// Function type used to customize element deallocation
typedef void (*free_element_fun_t) (void* to_free);

// Node type
typedef struct _set_node_t {
    void* data;
    struct _set_node_t* parent;
    struct _set_node_t* left;
    struct _set_node_t* right;
} set_node_t;

// Set type
typedef struct _set_t {
    set_node_t* root;
    compfun_t comp;
    handle_dup_fun_t handle_dup;
    free_element_fun_t free_element;
} set_t;

// Create a new node with no children
set_node_t* set_node_new(void* data, set_node_t* parent) {
    set_node_t* result = malloc(sizeof(set_node_t));
    result->parent = parent;
    result->left = NULL;
    result->right = NULL;
    result->data = data;
    return result;
}

// Check if given node has no children
int node_is_leaf(const set_node_t* node) {
    return node && node->right && node->left;
}

// Create a new empty set
set_t* set_empty(compfun_t comp, handle_dup_fun_t handle_dup, free_element_fun_t free_element) {
    set_t* result = malloc(sizeof(set_t));
    result->root = NULL;
    result->comp = comp;
    result->handle_dup = handle_dup;
    result->free_element = free_element;
    return result;
}

// Free memory allocated by given set
void node_free(set_node_t*, free_element_fun_t);
int set_free(set_t* set) {
    if (!set)
        return SET_ERR_NULL_SET;

    node_free(set->root, set->free_element); set->root = NULL;
    free(set);

    return SET_OK;
}
void node_free(set_node_t* node, free_element_fun_t free_ele) {
    if (node) {
        // Free data the node contains.
        free_ele(node->data);

        // Recursively free left and right nodes
        node_free(node->left, free_ele);
        node_free(node->right, free_ele);

        // Free memory occupied by node structure itself
        free(node);
    }
}

// Apply a given function to all nodes in the set
void node_walk(set_node_t* root, void (*func)(set_node_t**)) {
    if (root) {
        if (root->left)
            node_walk(root->left, func);

        func(&root);

        if (root->right)
            node_walk(root->right, func);
    }
}

// Print the contents of a set in order
void node_print(FILE*, set_node_t*);
int set_print(FILE* out_f, set_t* set) {
    if (!set)
        return SET_ERR_NULL_SET;

    fputs("(", out_f);

    node_print(out_f, set->root);

    fputs(")", out_f);

    return SET_OK;
}
// Helper function to aid recursion in set_print
void node_print(FILE* out_f, set_node_t* node) {
    if (node) {
        node_print(out_f, node->left);
        fprintf(out_f, "%s, ", (char*) node->data);
        node_print(out_f, node->right);
    }
}

// Insert element at rightmost node 
int node_add(set_node_t**, set_node_t*, compfun_t comp, handle_dup_fun_t, set_node_t*);
void node_insert_at_rightmost(set_node_t** root, set_node_t* to_insert) {
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
void node_insert_at_leftmost(set_node_t** root, set_node_t* to_insert) {
    if (root == NULL) {
        ERROR("Null pointer ref");
    }

    if (*root == NULL) {
        *root = to_insert;
        return;
    }

    node_insert_at_leftmost(&((*root)->left), to_insert);
}


// Add an element to the set
int set_add(set_t* set, void* element) {
    if (element == NULL)                            
        return SET_ERR_NULL_ELE;                    
    if (set == NULL)
        return SET_ERR_NULL_SET;

    set_node_t* node_to_add = set_node_new(element, NULL);
    int result = node_add(&(set->root), node_to_add, set->comp, set->handle_dup, NULL);

    if (result == SET_INSERTION_FAILED) {
        // TODO: Optimization opportunity: do not do this free
        node_free(node_to_add, set->free_element);
        return SET_INSERTION_FAILED;
    }

    return SET_OK;
}                                                   
// Helper function to aid recursion
//  NB. cur_parent is meant to be NULL at first call
int node_add(set_node_t** root_ref, set_node_t* to_add,
        compfun_t comp, handle_dup_fun_t handle_dup,
        set_node_t* cur_parent) {
    if (root_ref == NULL) {
        ERROR("Null pointer ref");
    }

    set_node_t* root = *root_ref;

    if (root == NULL) {
        to_add->parent = cur_parent;
        *root_ref = to_add;
        return SET_OK;
    } else {
        void* comped_ele = root->data;
        void* element_to_add = to_add->data;
        int comp_res = comp(element_to_add, comped_ele);
        if (comp_res == 0) {
            return handle_dup(comped_ele, element_to_add);
        } else if (comp_res < 0) {
            return node_add(&(root->left), to_add, comp, handle_dup, root);
        } else {
            return node_add(&(root->right), to_add, comp, handle_dup, root);
        }
    }
}

// Retrieves the specified element from a set. Returns NULL if there is no
// such element inside the set.
set_node_t** node_get_ref(set_node_t**, const void*, compfun_t);
set_node_t* node_get(set_node_t*, const void*, compfun_t);
void* set_get(set_t* set, const void* element) {
    // Search element starting from the set's root node using its associated comparison function.
    const set_node_t* wanted_node = node_get(set->root, element, set->comp);

    // A null node from node_get means the element was not found; return NULL in turn to indicate
    // the same thing.
    if (wanted_node == NULL)
        return NULL;

    // Element was found!
    return wanted_node->data;
}
// Helpers for node_get function.
set_node_t** node_get_ref(set_node_t** root_ref, const void* element, compfun_t comp) {  
    if (root_ref == NULL) {
        ERROR("NULL pointer ref");
    }

    set_node_t* root = *root_ref;

    if (!root) {
        return NULL;
    }

    int comp_result = comp(element, root->data);
    if (comp_result == 0)  
        return root_ref;
    else if (comp_result < 0)
        return node_get_ref(&(root->left), element, comp);
    else
        return node_get_ref(&(root->right), element, comp);
}
set_node_t* node_get(set_node_t* root, const void* element, compfun_t comp) {
    // TODO: Optimization chance: unnecessary ref/deref could be removed by allowing some code duplication.
    return *(node_get_ref(&root, element, comp));
}

// Remove an element from a given set and return it
set_node_t* node_remove(set_node_t**, compfun_t comp);
void set_remove(set_t* set, void* ele_to_remove) {
    node_free(
            node_remove(
                node_get_ref(&(set->root), ele_to_remove, set->comp),
                set->comp
            ),
            set->free_element
        );
}
set_node_t* node_remove(set_node_t** to_remove_ref, compfun_t comp) {
    if (!to_remove_ref) {
        return NULL;
    }

    set_node_t* to_remove = *to_remove_ref;                           

    // Get correct information                                        
    // TODO: optimization opportunity: do not choose tree albitrarily 
    set_node_t* substitute = NULL;                                    
    set_node_t** overlapping_top = NULL;                              
    set_node_t** overlapping_bottom = NULL;                           
    set_node_t** to_shift = NULL;                                     
    void (*overlap_insert)(set_node_t**, set_node_t*) = NULL;         
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

/**********************************************/
/* Data types used to construct relations set */
/**********************************************/
typedef struct rxing_amount_info_ {
    int amount;
    set_t* rxing_ents_set;    // set of const char*
} rxing_amount_info_t;
// Custom compare function
int rxing_amount_info_comp(const void* lhs, const void* rhs) {
    int lhs_id = ((rxing_amount_info_t*) lhs)->amount;
    int rhs_id = ((rxing_amount_info_t*) rhs)->amount;

    return lhs_id < rhs_id;
}
// Custom free function
void rxing_amount_info_free(void* to_free) {
    set_free(((rxing_amount_info_t*) to_free)->rxing_ents_set);

    free(to_free);
}

typedef struct rxing_ent_info_ {
    char* id;
    set_t* txing_ents_set;    // set of const char*
} rxing_ent_info_t;
// Custom compare function
int rxing_ent_info_comp(const void* lhs, const void* rhs) {
    const char* lhs_id = ((rxing_ent_info_t*) lhs)->id;
    const char* rhs_id = ((rxing_ent_info_t*) rhs)->id;

    return strcmp(lhs_id, rhs_id);
}
// Custom free function
void rxing_ent_info_free(void* to_free_v) {
    rxing_ent_info_t* to_free = (rxing_ent_info_t*) to_free_v;

    free(to_free->id);
    set_free(to_free->txing_ents_set);

    free(to_free);
}

typedef struct relinfo_t_ {
    char* id;
    set_t* rxing_ents_set;    // set of rxing_ent_info
    set_t* rxing_amounts_set; // set of rxing_amount_info
} relinfo_t;
// Custom compare function for relinfo type (used in sets)
int relinfo_comp(const void* lhs, const void* rhs) {
    const char* lhs_id = ((relinfo_t*) lhs)->id;
    const char* rhs_id = ((relinfo_t*) rhs)->id;

    return strcmp(lhs_id, rhs_id);
}
// Custom free function
void relinfo_free(void* to_free_v) {
    relinfo_t* to_free = (relinfo_t*) to_free_v;

    free(to_free->id);
    set_free(to_free->rxing_ents_set);
    set_free(to_free->rxing_amounts_set);

    free(to_free);
}

// Allocate new relinfo
relinfo_t* relinfo_new(char* rel_id, char* tx_ent, char* rx_ent) {
    relinfo_t* result = malloc(sizeof(relinfo_t));
    result->id = rel_id;
    result->rxing_ents_set = set_empty(&rxing_ent_info_comp, &disallow_duplicates, &rxing_ent_info_free);
    result->rxing_amounts_set = set_empty(&rxing_amount_info_comp, &disallow_duplicates, &rxing_amount_info_free);

    return result;
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
set_t* initialize_entities() {
    return set_empty(&strcomp, &discard_dup, &free);
}

// Initialize empty relations storage
set_t* initialize_relations() {
    return set_empty(&relinfo_comp, &disallow_duplicates, &relinfo_free);
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
    set_t* entities = initialize_entities();
    set_t* relations = initialize_relations();

    // User interaction loop
    char command[1024];
    while (1) {
        // Read head of command from user
        fscanf(in_f, "%s", command);

        // Process head of command
        if (strcmp(command, "addent") == 0) {
            // Allocate space for entity name
            char* to_add = malloc(sizeof(char) * ENT_NAME_BUF_LEN);

            // Parse second command argument as entity name
            fscanf(in_f, "%s", to_add);

            // Add entity to set
            set_add(entities, (void*) to_add);

        } else if (strcmp(command, "delent") == 0) {
            // Get name of entity to remove from first command argument
            char to_remove[ENT_NAME_BUF_LEN];
            fscanf(in_f, "%s", to_remove);

            // Perform removal
            set_remove(entities, (void*) to_remove);

        } else if (strcmp(command, "addrel") == 0) {
            // Get name of txing entity
            char* txing_ent = malloc(sizeof(char) * ENT_NAME_BUF_LEN);
            fscanf(in_f, "%s", txing_ent);

            // Get name of rxing entity
            char* rxing_ent = malloc(sizeof(char) * ENT_NAME_BUF_LEN);
            fscanf(in_f, "%s", rxing_ent);

            // Get name of relation
            char* relation = malloc(sizeof(char) * REL_NAME_BUF_LEN);
            fscanf(in_f, "%s", relation);

            // Perform insertion
            set_add(relations, relinfo_new(relation, txing_ent, rxing_ent));

        // Debug mode only commands
        } else if (debug_mode == DEBUG_ON) {
            if (strcmp(command, "gent") == 0) {
                // Retrieve string to get
                char to_get[ENT_NAME_BUF_LEN];
                fscanf(in_f, "%s", to_get);

                // Get it
                void* result = set_get(entities, (void*) to_get);

                // Print the result as a string if present
                if (result == NULL)
                    puts("NOT PRESENT!");
                else 
                    puts((const char*) result);

            } else if (strcmp(command, "pent") == 0) {
                set_print(out_f, entities);
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

    // Deallocate entity set
    set_free(entities);
    set_free(relations);

    // Close streams if necessary
    if (in_f != stdin)   fclose(in_f);
    if (out_f != stdout) fclose(out_f);

    // Exit
    return 0;
}
