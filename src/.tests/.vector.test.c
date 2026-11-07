/*
 * test_vector.c - simple tests for the Vector (sorted dynamic array of time_t)
 *
 * Build (from the project root; keep this file outside src/):
 *   gcc -O0 -Wall -Wextra -g -Isrc tests/test_vector.c src/DataTypes/Vector/Vector.c src/Timestamp/Timestamp.c -o build/test_vector
 *
 * Run:
 *   ./build/test_vector          (Linux)
 *   build\test_vector.exe        (Windows)
 *
 * Exit code: 0 if every check passes, 1 otherwise.
 */

#include <stdio.h>
#include <stdlib.h>
#include "DataTypes/Vector/Vector.h"
#include "Timestamp/Timestamp.h"

/* ---------------- mini test framework ---------------- */

static int checks = 0;
static int failed = 0;

#define CHECK(cond)                                                 \
    do {                                                            \
        checks++;                                                   \
        if (!(cond)) {                                              \
            failed++;                                               \
            printf("    FAIL line %d: %s\n", __LINE__, #cond);      \
        }                                                           \
    } while (0)

#define RUN(fn)                                                     \
    do {                                                            \
        int before = failed;                                        \
        fn();                                                       \
        printf("[%s] %s\n", failed == before ? " ok " : "FAIL", #fn); \
    } while (0)

/* ---------------- helpers ---------------- */

/* "YYYY-MM-DD HH:MM:SS" -> time_t (aborts the tests if the literal is bad) */
static time_t ts(const char *s)
{
    time_t t = 0;
    if (timestamp_parse(s, &t) != 0) {
        printf("    internal error: bad timestamp literal \"%s\"\n", s);
        exit(2);
    }
    return t;
}

/* true if the vector is strictly ascending and size <= capacity */
static int is_valid(const Vector *v)
{
    int i;
    if (v == NULL || v->data == NULL || v->size > v->capacity)
        return 0;
    for (i = 1; i < v->size; i++)
        if (timestamp_compare(v->data[i - 1], v->data[i]) >= 0)
            return 0;
    return 1;
}

/* xorshift: same sequence on Linux and Windows (rand() is not) */
static unsigned long long seed = 88172645463325252ULL;
static unsigned rnd(void)
{
    seed ^= seed << 13;
    seed ^= seed >> 7;
    seed ^= seed << 17;
    return (unsigned)(seed >> 32);
}

/* ---------------- tests ---------------- */

static void test_create(void)
{
    Vector *v;

    CHECK(vCreate(0) == NULL);
    CHECK(vCreate(-1) == NULL);

    v = vCreate(8);
    CHECK(v != NULL);
    if (v == NULL) return;
    CHECK(v->size == 0);
    CHECK(v->capacity == 8);
    CHECK(v->data != NULL);
    CHECK(v->data[0] == 0 && v->data[7] == 0);   /* zeroed */
    vDestroy(v);

    vDestroy(NULL);                              /* must not crash */
}

/* records arrive out of chronological order, must come out in order */
static void test_insert_out_of_order(void)
{
    Vector *v = vCreate(8);
    time_t out[8];
    int i;

    const char *arrival[] = {
        "2024-05-02 08:00:00", "2024-05-01 23:59:59", "2024-05-02 07:59:59",
        "2024-04-30 12:00:00", "2024-05-03 00:00:00"
    };
    const char *expected[] = {
        "2024-04-30 12:00:00", "2024-05-01 23:59:59", "2024-05-02 07:59:59",
        "2024-05-02 08:00:00", "2024-05-03 00:00:00"
    };

    CHECK(v != NULL);
    if (v == NULL) return;

    for (i = 0; i < 5; i++)
        CHECK(vInsert(v, ts(arrival[i])) == 1);

    CHECK(v->size == 5);
    CHECK(vListAscending(v, out) == 5);
    for (i = 0; i < 5; i++)
        CHECK(out[i] == ts(expected[i]));
    CHECK(is_valid(v));
    vDestroy(v);
}

/* start tiny and insert in the worst order: forces many reallocs */
static void test_insert_growth(void)
{
    Vector *v = vCreate(2);
    int i, bad = 0;

    CHECK(v != NULL);
    if (v == NULL) return;

    for (i = 1000; i >= 1; i--)
        if (vInsert(v, (time_t)i) != 1)
            bad++;

    CHECK(bad == 0);
    CHECK(v->size == 1000);
    CHECK(v->capacity >= 1000);
    CHECK(v->data[0] == 1 && v->data[999] == 1000);
    CHECK(is_valid(v));
    CHECK(vInsert(NULL, 1) == 0);
    vDestroy(v);
}

static void test_remove(void)
{
    Vector *v = vCreate(8);
    int i;

    CHECK(v != NULL);
    if (v == NULL) return;

    for (i = 1; i <= 5; i++)
        vInsert(v, (time_t)(i * 10));            /* 10 20 30 40 50 */

    CHECK(vRemove(v, 30) == 1);                  /* middle */
    CHECK(vRemove(v, 10) == 1);                  /* first */
    CHECK(vRemove(v, 50) == 1);                  /* last */
    CHECK(v->size == 2);
    CHECK(v->data[0] == 20 && v->data[1] == 40);

    CHECK(vRemove(v, 30) == 0);                  /* already removed */
    CHECK(vRemove(v, 25) == 0);                  /* never existed */
    CHECK(vRemove(v, 1) == 0 && vRemove(v, 99) == 0);
    CHECK(v->size == 2 && is_valid(v));

    CHECK(vRemove(v, 20) == 1);
    CHECK(vRemove(v, 40) == 1);
    CHECK(v->size == 0);
    CHECK(vRemove(v, 40) == 0);                  /* empty vector */
    CHECK(vInsert(v, 7) == 1 && v->size == 1);   /* still usable */

    CHECK(vRemove(NULL, 1) == 0);
    vDestroy(v);
}

static void test_get(void)
{
    Vector *v = vCreate(4);
    time_t out = 777;

    CHECK(v != NULL);
    if (v == NULL) return;

    vInsert(v, 30);
    vInsert(v, 10);
    vInsert(v, 20);

    CHECK(vGet(v, 0, &out) == 1 && out == 10);
    CHECK(vGet(v, 2, &out) == 1 && out == 30);

    out = 777;
    CHECK(vGet(v, -1, &out) == 0 && out == 777);
    CHECK(vGet(v, 3, &out) == 0 && out == 777);  /* index == size */
    CHECK(vGet(v, 0, NULL) == 0);
    CHECK(vGet(NULL, 0, &out) == 0);
    vDestroy(v);
}

static void test_list_ascending(void)
{
    Vector *v = vCreate(4);
    time_t out[4];

    CHECK(v != NULL);
    if (v == NULL) return;

    CHECK(vListAscending(v, out) == 0);          /* empty */

    vInsert(v, 30);
    vInsert(v, 10);
    vInsert(v, 20);
    CHECK(vListAscending(v, out) == 3);
    CHECK(out[0] == 10 && out[1] == 20 && out[2] == 30);

    CHECK(vListAscending(NULL, out) == 0);
    CHECK(vListAscending(v, NULL) == 0);
    vPrint(NULL);                                /* must not crash */
    vDestroy(v);
}

/* a bigger run, closer to what the benchmark will do */
#define BIG 20000
static void test_large_volume(void)
{
    const long long base = 1700000000LL, step = 37;
    int *order = malloc(BIG * sizeof *order);
    time_t *out = malloc(BIG * sizeof *out);
    Vector *v = vCreate(16);
    int i, j, bad;

    CHECK(order != NULL && out != NULL && v != NULL);
    if (order == NULL || out == NULL || v == NULL) {
        free(order); free(out); vDestroy(v);
        return;
    }

    for (i = 0; i < BIG; i++)                    /* shuffled arrival order */
        order[i] = i;
    for (i = BIG - 1; i > 0; i--) {
        j = (int)(rnd() % (unsigned)(i + 1));
        int t = order[i]; order[i] = order[j]; order[j] = t;
    }

    for (i = 0, bad = 0; i < BIG; i++)
        if (vInsert(v, (time_t)(base + order[i] * step)) != 1)
            bad++;
    CHECK(bad == 0 && v->size == BIG && is_valid(v));

    CHECK(vListAscending(v, out) == BIG);
    for (i = 0, bad = 0; i < BIG; i++)
        if (out[i] != (time_t)(base + i * step))
            bad++;
    CHECK(bad == 0);

    for (i = 0, bad = 0; i < BIG; i++)           /* remove the even ones */
        if (order[i] % 2 == 0 && vRemove(v, (time_t)(base + order[i] * step)) != 1)
            bad++;
    CHECK(bad == 0 && v->size == BIG / 2 && is_valid(v));

    CHECK(vListAscending(v, out) == BIG / 2);
    for (i = 0, bad = 0; i < BIG / 2; i++)       /* only the odd ones remain */
        if (out[i] != (time_t)(base + (2 * i + 1) * step))
            bad++;
    CHECK(bad == 0);

    free(order);
    free(out);
    vDestroy(v);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);            /* output survives a crash */
    puts("vector tests\n");

    RUN(test_create);
    RUN(test_insert_out_of_order);
    RUN(test_insert_growth);
    RUN(test_remove);
    RUN(test_get);
    RUN(test_list_ascending);
    RUN(test_large_volume);

    printf("\n%d checks, %d failed\n", checks, failed);
    return failed ? 1 : 0;
}