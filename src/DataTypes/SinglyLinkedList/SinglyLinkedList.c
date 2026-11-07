#include <stdio.h>
#include <stdlib.h>
#include "DataTypes/SinglyLinkedList/SinglyLinkedList.h"
#include "Timestamp/Timestamp.h"

/* Allocates one node. Internal helper: the node always ends up in a list. */
static SLNode *createNode(time_t value)
{
    SLNode *n = malloc(sizeof *n);
    if (n == NULL)
        return NULL;

    n->value = value;
    n->next = NULL;
    return n;
}

/* Merges two sorted lists without allocating memory. O(n). */
static SLNode *mergeSorted(SLNode *a, SLNode *b)
{
    SLNode dummy;
    SLNode *tail = &dummy;
    dummy.next = NULL;

    while (a != NULL && b != NULL)
    {
        if (timestamp_compare(a->value, b->value) <= 0)
        {
            tail->next = a;
            a = a->next;
        }
        else
        {
            tail->next = b;
            b = b->next;
        }
        tail = tail->next;
    }
    tail->next = (a != NULL) ? a : b; /* appends what is left */
    return dummy.next;
}

/* Top-down merge sort on a linked list. O(n log n), recursion depth O(log n). */
static SLNode *mergeSort(SLNode *head)
{
    if (head == NULL || head->next == NULL)
        return head; /* 0 or 1 node: already sorted */

    /* finds the middle with slow/fast pointers */
    SLNode *slow = head;
    SLNode *fast = head->next;
    while (fast != NULL && fast->next != NULL)
    {
        slow = slow->next;
        fast = fast->next->next;
    }

    SLNode *second = slow->next; /* cuts the list in two halves */
    slow->next = NULL;

    return mergeSorted(mergeSort(head), mergeSort(second));
}

SLinkedList *slCreate(void)
{
    SLinkedList *list = malloc(sizeof *list);
    if (list == NULL)
        return NULL;

    list->head = NULL;
    list->size = 0;
    return list;
}

void slDestroy(SLinkedList *list)
{
    if (list == NULL)
        return;

    SLNode *cur = list->head;
    while (cur != NULL)
    {
        SLNode *next = cur->next;
        free(cur);
        cur = next;
    }
    free(list);
}

int slInsert(SLinkedList *list, time_t value)
{
    if (list == NULL)
        return 0;

    SLNode *n = createNode(value);
    if (n == NULL)
        return 0;

    n->next = list->head;
    list->head = n;
    list->size++;
    return 1;
}

int slRemove(SLinkedList *list, time_t value)
{
    if (list == NULL || list->head == NULL)
        return 0; /* invalid or empty list */

    SLNode *prev = NULL;
    SLNode *cur = list->head;

    while (cur != NULL && timestamp_compare(cur->value, value) != 0)
    {
        prev = cur;
        cur = cur->next;
    }

    if (cur == NULL)
        return 0; /* value not found */

    if (prev == NULL)
        list->head = cur->next; /* removing the head */
    else
        prev->next = cur->next; /* removing from the middle or the end */

    free(cur);
    list->size--;
    return 1;
}

int slListAscending(SLinkedList *list, time_t *out)
{
    if (list == NULL || out == NULL)
        return 0;

    list->head = mergeSort(list->head); /* sort on listing */

    int i = 0;
    for (SLNode *cur = list->head; cur != NULL; cur = cur->next)
        out[i++] = cur->value;

    return i;
}

void slPrint(SLinkedList *list)
{
    if (list == NULL)
        return;

    list->head = mergeSort(list->head);

    char buf[TIMESTAMP_STR_SIZE];
    for (SLNode *cur = list->head; cur != NULL; cur = cur->next)
    {
        timestamp_to_string(cur->value, buf);
        printf("%s\n", buf);
    }
}