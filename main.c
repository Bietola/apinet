#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*************/
/* Utilities */
/*************/

// Prints error message and exists program
#define ERROR(msg) { \
    fprintf(stdout, "error in %s: %s", __func__ , msg); \
    exit(EXIT_FAILURE); \
}

// Custom, non type-safe, strcmp function that works nicely with the set_t interface
int strcomp(const void* lhs, const void* rhs) {
    return strcmp((const char*) lhs, (const char*) rhs);
}

// Custom duplication handling function for strings set that simply discards the newly inserted o
void discard_dup(void* old_ele, void* new_ele) {
    free((char*) new_ele);
}

// Custom duplication handling function that throws an error
// (needed for when duplicates should not be encountered)
void disallow_duplicates(void* old_ele, void* new_ele) {
    ERROR("ERR: Duplicates should not be allowed...\n");
}

/************************************************************************************/
/* Generic set datastructure. Implemented using a BST. Also used to represent sets. */
/************************************************************************************/

// Constants returned as part of error mechanisms
#define SET_OK           0
#define SET_ERR_NULL_SET 1
#define SET_ERR_NULL_ELE 2

// Function type used to customize set ordering
typedef int (*compfun_t)(const void* lhs, const void* rhs);

// Function type used to customize set duplicate handling
typedef void (*handle_dup_fun_t)(void* old, void* new);

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

// Substitute a node with another
set_node_t* node_substitute(set_node_t* to_substitute, set_node_t* to_substitute_with) {
    if (!to_substitute) {
        ERROR("Cannot substitute NULL node");
    }

    if (to_substitute_with) {
        // Parent of substituted becomes of the substitute
        to_substitute_with->parent = to_substitute->parent;

        // Same with left and right
        to_substitute_with->left = to_substitute->left;
        to_substitute_with->right = to_substitute->right;
    }

    // substituted node is detached from the tree
    to_substitute->parent = NULL;
    to_substitute->right = NULL;
    to_substitute->left = NULL;

    // Return newly substituted node as result
    return to_substitute_with;
}

// Create a new empty set
set_t* set_empty(compfun_t comp, handle_dup_fun_t handle_dup) {
    set_t* result = malloc(sizeof(set_t));
    result->root = NULL;
    result->comp = comp;
    result->handle_dup = handle_dup;
    return result;
}

// Free memory allocated by given set
void node_free(set_node_t*);
int set_free(set_t* set) {
    if (!set)
        return SET_ERR_NULL_SET;

    node_free(set->root); set->root = NULL;
    free(set);

    return SET_OK;
}
void node_free(set_node_t* node) {
    if (node) {
        // Free data the node contains.
        free(node->data); node->data = NULL;

        // Recursively free left and right nodes
        node_free(node->left); node->left = NULL;
        node_free(node->right); node->right = NULL;

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
set_node_t* node_add(set_node_t*, set_node_t*, compfun_t comp, handle_dup_fun_t, set_node_t*);
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

    set->root = node_add(set->root, set_node_new(element, NULL), set->comp, set->handle_dup, NULL);

    return SET_OK;
}                                                   
// Helper function to aid recursion
//  NB. cur_parent is meant to be NULL at first call
set_node_t* node_add(set_node_t* root, set_node_t* to_add,
        compfun_t comp, handle_dup_fun_t handle_dup,
        set_node_t* cur_parent) {
    if (root == NULL) {
        to_add->parent = cur_parent;
        return to_add;
    } else {
        void* comped_ele = root->data;
        void* element_to_add = to_add->data;
        int comp_res = comp(element_to_add, comped_ele);
        if (comp_res == 0) {
            handle_dup(comped_ele, element_to_add);
        } else if (comp_res < 0) {
            root->left = node_add(root->left, to_add, comp, handle_dup, root);
        } else {
            root->right = node_add(root->right, to_add, comp, handle_dup, root);
        }
        return root;
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
        ERROR("NULL ref");
    }

    set_node_t* root = *root_ref;

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

// Performs the union of @from into @into
//  NB. @from parameter might be destroyed
void node_inplace_union(set_node_t** into_ref, set_node_t* from, compfun_t comp) {
    if (!into_ref)
        ERROR("ERR: null pointer reference\n");

    set_node_t* into = *into_ref;

    if (!into) {
        *into_ref = from;
        return;
    }

    if (from) {
        node_inplace_union(into_ref, from->left, comp);
        node_inplace_union(into_ref, from->right, comp);
        *into_ref = node_add(into, set_node_new(from->data, NULL), comp, &disallow_duplicates, NULL);
    }
}                                                                        

// Remove an element from a given set and return it
set_node_t* node_remove(set_node_t**, compfun_t comp);
void set_remove(set_t* set, void* ele_to_remove) {
    node_free(
            node_remove(
                node_get_ref(&(set->root), ele_to_remove, set->comp),
                set->comp
            )
        );
}
set_node_t* node_remove_impl(
        set_node_t** to_remove,
        set_node_t* substitute,
        set_node_t** overlapping_top, set_node_t** overlapping_bottom,
        set_node_t** to_shift) {

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
set_node_t* node_remove(set_node_t** to_remove_ref, compfun_t comp) {
    if (!to_remove_ref) {
        ERROR("Null pointer ref");
    }

    if (to_remove->right) {
        node_remove_impl(
                to_remove_ref,
                &get_right,              // substitute
                &get_left, &get_right,   // overlapping_top, overlapping_bottom
                &get_left,               // to_shift
                &node_insert_at_leftmost // overlap_insert
        );
    } else {
        node_remove_impl(
                to_remove_ref,
                &get_left,                // substitute
                &get_right, &get_left,    // overlapping_top, overlapping_bottom
                &get_right,               // to_shift
                &node_insert_at_rightmost // overlap_insert
        );
    }

    /*********************************************************************/
    /* set_node_t* to_remove = *to_remove_ref;                           */
    /*                                                                   */
    /* if (!to_remove) {                                                 */
    /*     ERROR("Cannot remove null node");                             */
    /* }                                                                 */
    /*                                                                   */
    /* // Get correct information                                        */
    /* // TODO: optimization opportunity: do not choose tree albitrarily */
    /* set_node_t* substitute = NULL;                                    */
    /* set_node_t** overlapping_top = NULL;                              */
    /* set_node_t** overlapping_bottom = NULL;                           */
    /* set_node_t** to_shift = NULL;                                     */
    /* void (*overlap_insert)(set_node_t**, set_node_t*) = NULL;         */
    /* if (to_remove->right) {                                           */
    /*     substitute = to_remove->right;                                */
    /*     overlapping_top = &(to_remove->left);                         */
    /*     if (substitute) {                                             */
    /*         to_shift = &(substitute->right);                          */
    /*         overlapping_bottom = &(substitute->left);                 */
    /*     }                                                             */
    /*     overlap_insert = &node_insert_at_leftmost;                    */
    /* } else {                                                          */
    /*     substitute = to_remove->left;                                 */
    /*     overlapping_top = &(to_remove->right);                        */
    /*     if (substitute) {                                             */
    /*         overlapping_bottom = &(substitute->right);                */
    /*         to_shift = &(substitute->left);                           */
    /*     }                                                             */
    /*     overlap_insert = &node_insert_at_rightmost;                   */
    /* }                                                                 */
    /*********************************************************************/

}

/********/
/* Main */
/********/
int main(int argc, char** argv) {
    // Check if command was well formed
    if (argc < 3)
        puts("Please provide input and output files as arguments");
    if (argc > 3)
        puts("Too many arguments provided!");

    // Open files specified in first arguments
    FILE* in_f = fopen(argv[1], "r");
    FILE* out_f = fopen(argv[2], "w");

    // All of apinet's entities
    set_t* set  = set_empty(&strcomp, &discard_dup);
    set_t* set1 = set_empty(&strcomp, &discard_dup);

    // All of apinet's relations
    // TODO: set_t* relations = ...;

    // User interaction loop
    char command[1024];
    while (1) {
        // Read first section of command from user
        fscanf(in_f, "%s", command);

        if (strcmp(command, "add") == 0) {
            char* to_add = malloc(sizeof(char) * 1024);
            fscanf(in_f, "%s", to_add);
            set_add(set, (void*) to_add);
        } else if (strcmp(command, "remove") == 0) {
            char to_remove[1024];
            fscanf(in_f, "%s", to_remove);
            set_remove(set, (void*) to_remove);
        } else if (strcmp(command, "union") == 0) {
            node_inplace_union(&(set->root), set1->root, &strcomp);
        } else if (strcmp(command, "add1") == 0) {
            char* to_add = malloc(sizeof(char) * 1024);
            fscanf(in_f, "%s", to_add);
            set_add(set1, (void*) to_add);
        } else if (strcmp(command, "get") == 0) {
            // retrieve string to get
            char* to_get = malloc(sizeof(char) * 1024);
            fscanf(in_f, "%s", to_get);

            // get it
            void* result = set_get(set, (void*) to_get);

            // print the result as a string if present
            if (result == NULL)
                puts("NOT PRESENT!");
            else 
                puts((const char*) result);
        } else if (strcmp(command, "print") == 0) {
            set_print(out_f, set);
            fprintf(out_f, "\n");
        } else if (strcmp(command, "print1") == 0) {
            set_print(out_f, set1);
            fprintf(out_f, "\n");
        } else if (strcmp(command, "quit") == 0) {
            break;
        }
    }

    set_free(set); 
    set_free(set1); 

    fclose(in_f);
    fclose(out_f);

    return 0;
}
