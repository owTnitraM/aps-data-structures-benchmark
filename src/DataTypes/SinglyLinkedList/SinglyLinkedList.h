#ifndef SINGLY_LINKED_LIST_H
#define SINGLY_LINKED_LIST_H

// Apelidos para simplificar a keyword struct
typedef struct Node Node;
typedef struct SLinkedList SLinkedList;

// Definição de estruturas
struct Node
{
    struct Node *next;
    int value;
};

struct SLinkedList
{
    struct Node *head;
    struct Node *tail;
    int size;
};

#endif