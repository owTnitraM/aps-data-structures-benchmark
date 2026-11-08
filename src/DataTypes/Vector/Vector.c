#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "DataTypes/Vector/Vector.h"
#include "Timestamp/Timestamp.h"

/* Menor indice i tal que data[i] >= value (ou v->size). */
static int lowerBound(const Vector *v, time_t value)
{
    int lo = 0;
    int hi = v->size;

    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;

        if (timestamp_compare(v->data[mid], value) < 0)
            lo = mid + 1;
        else
            hi = mid;
    }

    return lo;
}

static int grow(Vector *v)
{
    int newCapacity;
    time_t *tmp;

    if (v->capacity > INT_MAX / 2)
        return 0;

    newCapacity = v->capacity * 2;
    tmp = realloc(v->data, (size_t)newCapacity * sizeof *tmp);
    if (tmp == NULL)
        return 0;

    v->data = tmp;
    v->capacity = newCapacity;

    return 1;
}

Vector *vCreate(int buffer)
{
    Vector *v;

    if (buffer < 1)
        return NULL;

    v = malloc(sizeof *v);
    if (v == NULL)
        return NULL;

    v->data = calloc((size_t)buffer, sizeof *v->data);
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

int vInsert(Vector *v, time_t value)
{
    int pos;

    if (v == NULL)
        return 0;

    if (v->size == v->capacity && !grow(v))
        return 0;

    pos = lowerBound(v, value);

    memmove(&v->data[pos + 1], &v->data[pos],
            (size_t)(v->size - pos) * sizeof *v->data);

    v->data[pos] = value;
    v->size++;

    return 1;
}

int vRemove(Vector *v, time_t value)
{
    int pos;

    if (v == NULL || v->size == 0)
        return 0;

    pos = lowerBound(v, value);
    if (pos == v->size || timestamp_compare(v->data[pos], value) != 0)
        return 0;

    memmove(&v->data[pos], &v->data[pos + 1],
            (size_t)(v->size - pos - 1) * sizeof *v->data);

    v->size--;

    return 1;
}

int vListAscending(const Vector *v, time_t *out)
{
    int i;

    if (v == NULL || out == NULL)
        return 0;

    for (i = 0; i < v->size; i++)
        out[i] = v->data[i];

    return v->size;
}

void vPrint(const Vector *v)
{
    char buf[TIMESTAMP_STR_SIZE];
    int i;

    if (v == NULL)
        return;

    for (i = 0; i < v->size; i++) {
        timestamp_to_string(v->data[i], buf);
        printf("%s\n", buf);
    }
}
