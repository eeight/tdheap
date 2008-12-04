#include <stdio.h>
#include <malloc.h>

typedef struct _Node
{
    struct _Node *prev, *next;
    int v;
} Node;

Node *new_node(int v, Node *prev, Node *next) {
    Node *node = malloc(sizeof(*node));

    node->prev = prev;
    node->next = next;
    node->v = v;
    return node;
}

void add_value(Node **head, Node **tail, int v) {
    if (*head == 0) {
        *head = new_node(v, 0, 0);
        *tail = *head;
    } else {
        (*tail)->next = new_node(v, *tail, 0);
        (*tail) = (*tail)->next;
    }
}

void free_list(Node **head, Node **tail) {
    Node *i;

    for (i = *head; i != 0;) {
        Node *next = i->next;
        free(i);
        i = next;
    }

    *head = 0;
    *tail = 0;
}

int main(void) {
    int next;
    Node *head = 0, *tail = 0;
    Node *i;

    while (scanf("%d", &next) == 1) {
        add_value(&head, &tail, next);
    }

    for (i = tail; i != 0; i = i->prev) {
        printf("%d\n", i->v);
    }

    free_list(&head, &tail);

    return 0;
}
