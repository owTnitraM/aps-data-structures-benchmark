#ifndef SINGLY_LINKED_LIST_H
#define SINGLY_LINKED_LIST_H

#include <time.h>

typedef struct SLNode SLNode;
typedef struct SLinkedList SLinkedList;

struct SLNode {
    SLNode *next;
    time_t value;
};

struct SLinkedList {
    SLNode *head;
    int size;
};

SLinkedList *slCreate(void);
void slDestroy(SLinkedList *list);

int slInsert(SLinkedList *list, time_t value);
int slRemove(SLinkedList *list, time_t value);
int slListAscending(SLinkedList *list, time_t *out);

void slPrint(SLinkedList *list);

#endif
