#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*************/
/* Utilities */
/*************/

// Custom, non type-safe, strcmp function that works nicely with the set_t interface
int strcomp(const void* lhs, const void* rhs) {
    return strcmp((const char*) lhs, (const char*) rhs);
}

// Custom duplication handling function for strings set that simply discards the newly inserted o
void discard_dup(void* old_ele, void* new_ele) {
    free((char*) new_ele);
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
set_node_t* set_node_new(void* data) {
    set_node_t* result = malloc(sizeof(set_node_t));
    result->left = NULL;
    result->right = NULL;
    result->data = data;
    return result;
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
        free(node->data); node->data = NULL;
        node_free(node->left); node->left = NULL;
        node_free(node->right); node->right = NULL;
    }
}

// Print the contents of a set in order
void node_print(set_node_t*);
int set_print(set_t* set) {
    if (!set)
        return SET_ERR_NULL_SET;

    printf("(");

    node_print(set->root);

    printf(")");

    return SET_OK;
}
// Helper function to aid recursion
void node_print(set_node_t* node) {
    if (node) {
        if (node->left)
            node_print(node->left);

        printf("%s, ", (const char*) node->data);

        if (node->right)
            node_print(node->right);
    }
}

// Add an element to the set
void node_add(set_node_t**, void*, compfun_t comp, handle_dup_fun_t);
int set_add(set_t* set, void* element) {
    if (element == NULL)                            
        return SET_ERR_NULL_ELE;                    
    if (set == NULL)
        return SET_ERR_NULL_SET;

    node_add(&(set->root), element, set->comp, set->handle_dup);

    return SET_OK;
}                                                   
// Helper function to aid recursion
void node_add(set_node_t** node, void* element, compfun_t comp, handle_dup_fun_t handle_dup) {
    if (*node == NULL) {
        *node = set_node_new(element);
    } else {
        void* comped_ele = (*node)->data;
        int comp_res = comp(element, comped_ele);
        if (comp_res == 0) {
            handle_dup(comped_ele, element);
        } else if (comp_res < 0) {
            node_add(&((*node)->left), element, comp, handle_dup);
        } else {
            node_add(&((*node)->right), element, comp, handle_dup);
        }
    }
}

// Retrieves the specified element from a set. Returns NULL if there is no
// such element inside the set.
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
// Helper for node_get function.
set_node_t* node_get(set_node_t* node, const void* element, compfun_t comp) {  
    if (node == NULL)  
        return NULL;  

    int comp_result = comp(element, node->data);
    if (comp_result == 0)  
        return node;
    else if (comp_result < 0)
        return node_get(node->left, element, comp);
    else
        return node_get(node->right, element, comp);
}

/********/
/* Main */
/********/
int main() {
    set_t* set = set_empty(&strcomp, &discard_dup);

    char command[1024];
    while (1) {
        printf("> ");
        scanf("%s", command);

        if (strcmp(command, "add") == 0) {
            // retrieve string to add
            char* toAdd = malloc(sizeof(char) * 1024);
            scanf("%s", toAdd);

            // add it to set
            int err = set_add(set, (void*) toAdd);

            // check for errors
            if (err == SET_ERR_NULL_ELE) {
                assert(NULL);
            }
        } else if (strcmp(command, "get") == 0) {
            // retrieve string to get
            char* to_get = malloc(sizeof(char) * 1024);
            scanf("%s", to_get);

            // get it
            void* result = set_get(set, (void*) to_get);

            // print the result as a string if present
            if (result == NULL)
                puts("NOT PRESENT!");
            else 
                puts((const char*) result);
        } else if (strcmp(command, "print") == 0) {
            set_print(set);
            printf("\n");
        } else if (strcmp(command, "quit") == 0) {
            break;
        }
    }

    set_free(set);

    return 0;
}
