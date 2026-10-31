#ifndef VECTOR_H
#define VECTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct Vector Vector;

/* struct Vector
{
    time_t *data; // points to element 0 of the array
    int size;     // how many elements are in use
    int capacity; // how many fit in the array
};

Vector *VCreate(int buffer)
{
    if (buffer <= 0)
        return NULL;

    Vector *v = malloc(sizeof(Vector));
    if (!v)
        return NULL;

    v->data = calloc(buffer, sizeof(time_t));
    if (!v->data)
    { // second allocation failed: free the first one
        free(v);
        return NULL;
    }

    v->capacity = buffer;
    v->size = 0;
    return v;
}

int VGet(const Vector *v, int index, time_t *out)
{
    if (!v || !out || index < 0 || index >= v->size)
        return 0;
    *out = v->data[index];
    return 1;
}

int VSet(Vector *v, int index, time_t value)
{
    if (!v || index < 0 || index >= v->size)
        return 0;
    v->data[index] = value;
    return 1;
}

int VInsert(Vector *v, int index, time_t value)
{
    if (!v)
        return 0;
} */

#endif