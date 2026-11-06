#ifndef SINGLY_LINKED_LIST_H
#define SINGLY_LINKED_LIST_H

#include <time.h>

typedef struct SLNode SLNode;
typedef struct SLinkedList SLinkedList;

struct SLNode
{
    SLNode *next;
    time_t value;
};

/*
 * Unsorted singly linked list of timestamps ("sort on listing" strategy):
 * inserting is cheap (at the head), the ordering cost is paid when listing.
 */
struct SLinkedList
{
    SLNode *head;
    int size; // how many nodes the list has
};

/* Allocates an empty list. Returns NULL if the allocation fails. */
SLinkedList *slCreate(void);

/* Frees every node and the list itself. Accepts NULL. */
void slDestroy(SLinkedList *list);

/*
 * Inserts `value` at the head of the list. Does not check for duplicates
 * (that would cost O(n)); the project assumes unique keys.
 * Returns 1 on success and 0 on failure (list is NULL or memory ran out).
 * O(1) in every case.
 */
int slInsert(SLinkedList *list, time_t value);

/*
 * Removes the node holding `value` (linear search).
 * Returns 1 if it was removed and 0 if not found (or list is NULL/empty).
 * Best case O(1) (value at the head), worst case O(n).
 */
int slRemove(SLinkedList *list, time_t value);

/*
 * Sorts the list with merge sort and copies all records, in ascending order,
 * into `out`, which must have room for list->size elements.
 * Returns how many were copied (0 if list/out is NULL).
 * O(n log n) in every case. Does no I/O, so it can be timed in the benchmark.
 * Side effect: the list itself is left sorted (the sort is done in place).
 */
int slListAscending(SLinkedList *list, time_t *out);

/*
 * Sorts the list (same side effect as slListAscending) and prints the
 * records, one per line (demo/debug only).
 */
void slPrint(SLinkedList *list);

#endif