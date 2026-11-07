#ifndef VECTOR_H
#define VECTOR_H

#include <time.h>

typedef struct Vector Vector;

/*
 * Dynamic array of timestamps kept in ascending order ("ordered insertion"
 * strategy): listing is just a traversal, the cost is paid on insert/remove.
 */
struct Vector
{
    time_t *data; // points to element 0 of the array
    int size;     // how many elements are in use
    int capacity; // how many fit in the array
};

/*
 * Allocates a vector with room for `buffer` elements (zeroed).
 * Returns NULL if buffer < 1 or if the allocation fails.
 */
Vector *vCreate(int buffer);

/* Frees v->data and the vector itself. Accepts NULL. */
void vDestroy(Vector *v);

/*
 * Safe alternative to direct access (v->data[i]).
 * Copies the element at `index` into *out.
 * Returns 1 on success and 0 if v/out is NULL or index is not in [0, size).
 */
int vGet(const Vector *v, int index, time_t *out);

/*
 * Inserts `value` keeping the vector in ascending order: binary search for
 * the position, then shifts the elements to the right of it.
 * Doubles the capacity when full. Assumes unique keys (an equal key would be
 * placed before the existing one).
 * Returns 1 on success and 0 on failure (v is NULL or memory ran out).
 * Best case O(log n) (insert at the end), worst case O(n) (insert at the start).
 */
int vInsert(Vector *v, time_t value);

/*
 * Removes `value`: binary search, then shifts the elements to the left.
 * Returns 1 if it was removed and 0 if not found (or v is NULL/empty).
 * Best case O(log n) (remove the last), worst case O(n) (remove the first).
 */
int vRemove(Vector *v, time_t value);

/*
 * Copies all records, in ascending order, into `out`, which must have room
 * for v->size elements. Returns how many were copied (0 if v/out is NULL).
 * O(n) in every case. Does no I/O, so it can be timed in the benchmark.
 */
int vListAscending(const Vector *v, time_t *out);

/* Prints the records in ascending order, one per line (demo/debug only). */
void vPrint(const Vector *v);

#endif