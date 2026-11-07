#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "DataTypes/Vector/Vector.h"
#include "Timestamp/Timestamp.h"

/*
 * Binary search: returns the smallest index i such that data[i] >= value
 * (or v->size if every element is smaller). O(log n).
 * Used both to find the insertion position and to locate the element
 * on removal.
 */
static int lowerBound(const Vector *v, time_t value)
{
    int lo = 0;
    int hi = v->size;                       /* interval [lo, hi) */

    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;       /* avoids overflow of lo + hi */
        if (timestamp_compare(v->data[mid], value) < 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

/* Doubles the capacity. Returns 1 on success and 0 on failure (v stays valid). */
static int grow(Vector *v)
{
    if (v->capacity > INT_MAX / 2)
        return 0;                           /* the double does not fit in an int */

    int newCapacity = v->capacity * 2;
    time_t *tmp = realloc(v->data, (size_t)newCapacity * sizeof *tmp);
    if (tmp == NULL)
        return 0;                           /* the old v->data is still valid */

    v->data = tmp;
    v->capacity = newCapacity;
    return 1;
}

Vector *vCreate(int buffer)
{
    if (buffer < 1)
        return NULL;

    Vector *v = malloc(sizeof *v);
    if (v == NULL)
        return NULL;

    v->data = calloc((size_t)buffer, sizeof *v->data);   /* zeroed values */
    if (v->data == NULL) {
        free(v);
        return NULL;
    }

    v->size = 0;
    v->capacity = buffer;
    return v;
}

void vDestroy(Vector *v)
{
    if (v == NULL)
        return;
    free(v->data);
    free(v);
}

int vGet(const Vector *v, int index, time_t *out)
{
    if (v == NULL || out == NULL || index < 0 || index >= v->size)
        return 0;

    *out = v->data[index];
    return 1;
}

/*
 * Binary search O(log n) + memmove O(n) = O(n).
 * Best case: insert at the end (nothing to shift) -> O(log n).
 * Worst case: insert at the start (shifts all n elements) -> O(n).
 */
int vInsert(Vector *v, time_t value)
{
    if (v == NULL)
        return 0;

    if (v->size == v->capacity && !grow(v))
        return 0;

    int pos = lowerBound(v, value);

    /* opens a gap by shifting [pos, size) one slot to the right */
    memmove(&v->data[pos + 1], &v->data[pos],
            (size_t)(v->size - pos) * sizeof *v->data);

    v->data[pos] = value;
    v->size++;
    return 1;
}

/*
 * Binary search O(log n) + memmove O(n) = O(n).
 * Best case: remove the last element (nothing to shift) -> O(log n).
 * Worst case: remove the first (shifts n-1 elements) -> O(n).
 */
int vRemove(Vector *v, time_t value)
{
    if (v == NULL || v->size == 0)
        return 0;

    int pos = lowerBound(v, value);
    if (pos == v->size || timestamp_compare(v->data[pos], value) != 0)
        return 0;                           /* value not found */

    /* closes the gap by shifting (pos, size) one slot to the left */
    memmove(&v->data[pos], &v->data[pos + 1],
            (size_t)(v->size - pos - 1) * sizeof *v->data);

    v->size--;
    return 1;
}

/* The vector is already sorted, so it is enough to traverse it: O(n). */
int vListAscending(const Vector *v, time_t *out)
{
    if (v == NULL || out == NULL)
        return 0;

    for (int i = 0; i < v->size; i++)
        out[i] = v->data[i];

    return v->size;
}

void vPrint(const Vector *v)
{
    if (v == NULL)
        return;

    char buf[TIMESTAMP_STR_SIZE];
    for (int i = 0; i < v->size; i++) {
        timestamp_to_string(v->data[i], buf);
        printf("%s\n", buf);
    }
}