/* Testes da lista simplesmente encadeada. */

#include <stdio.h>
#include <stdlib.h>
#include "DataTypes/SinglyLinkedList/SinglyLinkedList.h"
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

/* number of nodes reachable from head (stops at 'limit' to avoid loops) */
static int chain_length(const SLinkedList *list, int limit)
{
    int n = 0;
    const SLNode *cur;
    for (cur = list->head; cur != NULL && n <= limit; cur = cur->next)
        n++;
    return n;
}

/* the chain has exactly 'size' nodes (no lost or looping nodes) */
static int is_consistent(const SLinkedList *list)
{
    return list != NULL && chain_length(list, list->size + 1) == list->size;
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

static void test_create_destroy(void)
{
    SLinkedList *l = slCreate();

    CHECK(l != NULL);
    if (l == NULL) return;
    CHECK(l->head == NULL);
    CHECK(l->size == 0);
    slDestroy(l);

    slDestroy(NULL);                             /* must not crash */

    l = slCreate();                              /* destroy with nodes inside */
    slInsert(l, 1);
    slInsert(l, 2);
    slInsert(l, 3);
    slDestroy(l);                                /* leak check: use -fsanitize=address */
}

static void test_insert(void)
{
    SLinkedList *l = slCreate();
    int i, bad = 0;

    CHECK(l != NULL);
    if (l == NULL) return;

    CHECK(slInsert(NULL, 1) == 0);
    for (i = 1; i <= 100; i++)
        if (slInsert(l, (time_t)i) != 1)
            bad++;

    CHECK(bad == 0);
    CHECK(l->size == 100);
    CHECK(is_consistent(l));
    slDestroy(l);
}

/* values past January 2038 do not fit in a 32-bit int: node must hold a time_t */
static void test_value_is_full_time_t(void)
{
    SLinkedList *l = slCreate();
    time_t out[3];

    CHECK(l != NULL);
    if (l == NULL) return;

    slInsert(l, ts("2040-01-01 00:00:00"));
    slInsert(l, ts("2020-01-01 00:00:00"));
    slInsert(l, ts("2030-06-15 12:30:00"));

    CHECK(slListAscending(l, out) == 3);
    CHECK(out[0] == ts("2020-01-01 00:00:00"));
    CHECK(out[1] == ts("2030-06-15 12:30:00"));
    CHECK(out[2] == ts("2040-01-01 00:00:00"));
    CHECK(slRemove(l, ts("2040-01-01 00:00:00")) == 1);
    CHECK(l->size == 2);
    slDestroy(l);
}

static void test_remove(void)
{
    SLinkedList *l = slCreate();
    time_t out[8];
    int i;

    CHECK(l != NULL);
    if (l == NULL) return;

    CHECK(slRemove(l, 1) == 0);                  /* empty list */
    CHECK(slRemove(NULL, 1) == 0);

    for (i = 1; i <= 5; i++)
        slInsert(l, (time_t)(i * 10));           /* 10 20 30 40 50 (50 is the head) */

    CHECK(slRemove(l, 50) == 1);                 /* head */
    CHECK(slRemove(l, 10) == 1);                 /* tail (last node) */
    CHECK(slRemove(l, 30) == 1);                 /* middle */
    CHECK(l->size == 2 && is_consistent(l));

    CHECK(slRemove(l, 30) == 0);                 /* already removed */
    CHECK(slRemove(l, 25) == 0);                 /* never existed */
    CHECK(l->size == 2);

    CHECK(slListAscending(l, out) == 2);
    CHECK(out[0] == 20 && out[1] == 40);

    CHECK(slRemove(l, 20) == 1);
    CHECK(slRemove(l, 40) == 1);                 /* now empty */
    CHECK(l->size == 0 && l->head == NULL);
    CHECK(slInsert(l, 7) == 1 && l->size == 1);  /* still usable */
    slDestroy(l);
}

/* records arrive out of chronological order, must come out in order */
static void test_list_ascending(void)
{
    SLinkedList *l = slCreate();
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

    CHECK(l != NULL);
    if (l == NULL) return;

    CHECK(slListAscending(l, out) == 0);         /* empty list */

    for (i = 0; i < 5; i++)
        slInsert(l, ts(arrival[i]));

    CHECK(slListAscending(l, out) == 5);
    for (i = 0; i < 5; i++)
        CHECK(out[i] == ts(expected[i]));

    CHECK(l->size == 5 && is_consistent(l));     /* no node lost by the sort */
    CHECK(slListAscending(l, out) == 5);         /* listing twice gives the same */
    for (i = 0; i < 5; i++)
        CHECK(out[i] == ts(expected[i]));

    slInsert(l, ts("2024-05-01 00:00:00"));      /* insert after a listing */
    CHECK(slListAscending(l, out) == 6);
    CHECK(out[1] == ts("2024-05-01 00:00:00"));

    CHECK(slListAscending(NULL, out) == 0);
    CHECK(slListAscending(l, NULL) == 0);
    slPrint(NULL);                               /* must not crash */
    slDestroy(l);
}

/* a bigger run, closer to what the benchmark will do */
#define BIG 20000
static void test_large_volume(void)
{
    const long long base = 1700000000LL, step = 37;
    int *order = malloc(BIG * sizeof *order);
    time_t *out = malloc(BIG * sizeof *out);
    SLinkedList *l = slCreate();
    int i, j, bad;

    CHECK(order != NULL && out != NULL && l != NULL);
    if (order == NULL || out == NULL || l == NULL) {
        free(order); free(out); slDestroy(l);
        return;
    }

    for (i = 0; i < BIG; i++)                    /* shuffled arrival order */
        order[i] = i;
    for (i = BIG - 1; i > 0; i--) {
        int t;
        j = (int)(rnd() % (unsigned)(i + 1));
        t = order[i]; order[i] = order[j]; order[j] = t;
    }

    for (i = 0, bad = 0; i < BIG; i++)
        if (slInsert(l, (time_t)(base + order[i] * step)) != 1)
            bad++;
    CHECK(bad == 0 && l->size == BIG && is_consistent(l));

    CHECK(slListAscending(l, out) == BIG);
    for (i = 0, bad = 0; i < BIG; i++)
        if (out[i] != (time_t)(base + i * step))
            bad++;
    CHECK(bad == 0);

    for (i = 0, bad = 0; i < BIG; i++)           /* remove the even ones */
        if (order[i] % 2 == 0 && slRemove(l, (time_t)(base + order[i] * step)) != 1)
            bad++;
    CHECK(bad == 0 && l->size == BIG / 2 && is_consistent(l));

    CHECK(slListAscending(l, out) == BIG / 2);
    for (i = 0, bad = 0; i < BIG / 2; i++)       /* only the odd ones remain */
        if (out[i] != (time_t)(base + (2 * i + 1) * step))
            bad++;
    CHECK(bad == 0);

    free(order);
    free(out);
    slDestroy(l);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);            /* output survives a crash */
    puts("singly linked list tests\n");

    RUN(test_create_destroy);
    RUN(test_insert);
    RUN(test_value_is_full_time_t);
    RUN(test_remove);
    RUN(test_list_ascending);
    RUN(test_large_volume);

    printf("\n%d checks, %d failed\n", checks, failed);
    return failed ? 1 : 0;
}