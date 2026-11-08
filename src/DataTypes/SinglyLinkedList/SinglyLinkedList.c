#include <stdio.h>
#include <stdlib.h>
#include "DataTypes/SinglyLinkedList/SinglyLinkedList.h"
#include "Timestamp/Timestamp.h"

static SLNode *createNode(time_t value)
{
    SLNode *n = malloc(sizeof *n);

    if (n == NULL)
        return NULL;

    n->value = value;
    n->next = NULL;

    return n;
}

/* Junta duas listas ja ordenadas (O(n)). */
static SLNode *mergeSorted(SLNode *a, SLNode *b)
{
    SLNode dummy;
    SLNode *tail = &dummy;

    dummy.next = NULL;

    while (a != NULL && b != NULL) {
        if (timestamp_compare(a->value, b->value) <= 0) {
            tail->next = a;
            a = a->next;
        } else {
            tail->next = b;
            b = b->next;
        }

        tail = tail->next;
    }

    tail->next = (a != NULL) ? a : b;

    return dummy.next;
}

/* Merge sort top-down na lista (O(n log n)). */
static SLNode *mergeSort(SLNode *head)
{
    SLNode *slow;
    SLNode *fast;
    SLNode *second;

    if (head == NULL || head->next == NULL)
        return head;

    slow = head;
    fast = head->next;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }

    second = slow->next;
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
    SLNode *cur;

    if (list == NULL)
        return;

    cur = list->head;
    while (cur != NULL) {
        SLNode *next = cur->next;

        free(cur);
        cur = next;
    }

    free(list);
}

int slInsert(SLinkedList *list, time_t value)
{
    SLNode *n;

    if (list == NULL)
        return 0;

    n = createNode(value);
    if (n == NULL)
        return 0;

    n->next = list->head;
    list->head = n;
    list->size++;

    return 1;
}

int slRemove(SLinkedList *list, time_t value)
{
    SLNode *prev = NULL;
    SLNode *cur;

    if (list == NULL || list->head == NULL)
        return 0;

    cur = list->head;

    while (cur != NULL && timestamp_compare(cur->value, value) != 0) {
        prev = cur;
        cur = cur->next;
    }

    if (cur == NULL)
        return 0;

    if (prev == NULL)
        list->head = cur->next;
    else
        prev->next = cur->next;

    free(cur);
    list->size--;

    return 1;
}

int slListAscending(SLinkedList *list, time_t *out)
{
    SLNode *cur;
    int i = 0;

    if (list == NULL || out == NULL)
        return 0;

    list->head = mergeSort(list->head);

    for (cur = list->head; cur != NULL; cur = cur->next)
        out[i++] = cur->value;

    return i;
}

void slPrint(SLinkedList *list)
{
    char buf[TIMESTAMP_STR_SIZE];
    SLNode *cur;

    if (list == NULL)
        return;

    list->head = mergeSort(list->head);

    for (cur = list->head; cur != NULL; cur = cur->next) {
        timestamp_to_string(cur->value, buf);
        printf("%s\n", buf);
    }
}
