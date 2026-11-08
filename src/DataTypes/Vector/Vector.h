#ifndef VECTOR_H
#define VECTOR_H

#include <time.h>

typedef struct Vector Vector;

struct Vector {
    time_t *data;
    int size;
    int capacity;
};

Vector *vCreate(int buffer);
void vDestroy(Vector *v);

int vGet(const Vector *v, int index, time_t *out);
int vInsert(Vector *v, time_t value);
int vRemove(Vector *v, time_t value);
int vListAscending(const Vector *v, time_t *out);

void vPrint(const Vector *v);

#endif
