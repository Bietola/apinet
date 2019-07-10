#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**********************************************************/
/* Set of strings datastructure. Implemented using a BST. */
/**********************************************************/

// Constants returned as part of error mechanisms
#define SET_OK           0;
#define SET_ERR_NULL_SET 1;
#define SET_ERR_NULL_ELE 2;

// Node type
typedef struct _set_node_t {
    const char* data;
    struct _set_node_t* left;
    struct _set_node_t* right;

} set_node_t;

// Set type
typedef struct _set_t {
    set_node_t* root;
} set_t;

// Create a new node with no children
set_node_t* set_node_new(const char* data) {
    set_node_t* result = malloc(sizeof(set_node_t));
    result->left = NULL;
    result->right = NULL;
    result->data = data;
    return result;
}

// Create a new empty set
set_t* set_empty() {
    set_t* result = malloc(sizeof(set_t));
    result->root = NULL;
    return result;
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
        if (node->right)
            node_print(node->right);

        printf("%s, ", node->data);
    }
}
                                                     
// Add an element to the set
void node_add(set_node_t**, const char*);
int set_add(set_t* set, const char* element) {
    if (element == NULL)                            
        return SET_ERR_NULL_ELE;                    
    if (set == NULL)
        return SET_ERR_NULL_SET;

    node_add(&(set->root), element);

    return SET_OK;
}                                                   
// Helper function to aid recursion
void node_add(set_node_t** node, const char* element) {
    if (*node == NULL) {
        *node = set_node_new(element);
    } else if (strcmp(element, (*node)->data) < 0) {
        node_add(&((*node)->left), element);
    } else {
        node_add(&((*node)->right), element);               
    }
}

/********/
/* Main */
/********/
int main() {
    set_t* set = set_empty();

    set_add(set, "a");
    set_add(set, "b");
    set_add(set, "c");

    set_print(set);
    printf("\n");;

    return 0;
}
