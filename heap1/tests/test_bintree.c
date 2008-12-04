#include <stdio.h>
#include <malloc.h>

typedef struct _Node {
    struct _Node *left, *right;
    int v;
} Node;

Node *new_node(int v) {
    Node *node = malloc(sizeof(*node));

    node->v = v;
    node->left = 0;
    node->right = 0;

    return node;
}

void tree_insert(Node **root, int v) {
    if (*root == 0) {
        *root = new_node(v);
    } else {
        if ((*root)->v > v) {
            tree_insert(&(*root)->left, v);
        } else {
            tree_insert(&(*root)->right, v);
        }
    }
}

void free_tree(Node **root) {
    if (*root != 0) {
        free_tree(&(*root)->left);
        free_tree(&(*root)->right);
        free(*root);
        *root = 0;
    }
}

void traverse_tree(Node *root, void (*callback)(int)) {
    if (root != 0) {
        traverse_tree(root->left, callback);
        (*callback)(root->v);
        traverse_tree(root->right, callback);
    }
}

void print_int(int v) {
    printf("%d\n", v);
}

int main(void) {
    int next;
    Node *root = 0;

    while (scanf("%d", &next) == 1) {
        tree_insert(&root, next);
    }

    traverse_tree(root, &print_int);

    free_tree(&root);

    return 0;
}
