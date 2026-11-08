/*
 * test_avl.c - simple tests for the AVLTree
 *
 * Build (from the project root; keep this file outside src/):
 *   gcc -O0 -Wall -Wextra -g -Isrc tests/test_avl.c src/DataTypes/AVLTree/AVLTree.c src/Timestamp/Timestamp.c -o build/test_avl
 *
 * Run:
 *   ./build/test_avl          (Linux)
 *   build\test_avl.exe        (Windows)
 *
 * Exit code: 0 if every check passes, 1 otherwise.
 */

#include <stdio.h>
#include <stdlib.h>
#include "DataTypes/AVLTree/AVLTree.h"
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

/*
 * Returns the subtree height, or -1 if a rule is broken:
 * keys strictly inside (lo, hi) = BST order, |balance| <= 1 = AVL rule,
 * and the stored height is the real one. Counts the nodes it visits.
 */
static int check_node(const AVLNode *n, long long lo, long long hi, size_t *count)
{
    int hl, hr, h;

    if (n == NULL)
        return 0;
    if ((long long)n->value <= lo || (long long)n->value >= hi)
        return -1;

    hl = check_node(n->left, lo, (long long)n->value, count);
    hr = check_node(n->right, (long long)n->value, hi, count);
    if (hl < 0 || hr < 0 || hl - hr > 1 || hr - hl > 1)
        return -1;

    h = 1 + (hl > hr ? hl : hr);
    if (n->height != h)
        return -1;

    (*count)++;
    return h;
}

/* valid BST + valid AVL + the 'size' field matches the real node count */
static int is_valid(const AVLTree *t)
{
    size_t count = 0;

    if (t == NULL)
        return 0;
    return check_node(t->root, -(1LL << 62), 1LL << 62, &count) >= 0
           && count == t->size;
}

/* AVL height is at most ~1.44*log2(n+2); this integer version is a bit looser */
static int height_limit(size_t n)
{
    int log2n = 0;
    size_t m = n + 2;

    while (m > 1) {
        m >>= 1;
        log2n++;
    }
    return (3 * log2n) / 2 + 1;
}

/* the tree lists exactly expected[0..n) in ascending order (n <= 1024) */
static int list_is(const AVLTree *t, const long long *expected, size_t n)
{
    time_t out[1024];
    size_t i;

    if (avlListAscending(t, out, 1024) != n)
        return 0;
    for (i = 0; i < n; i++)
        if ((long long)out[i] != expected[i])
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

static void test_create_destroy(void)
{
    AVLTree *t = avlCreate();
    AVLTree s;

    CHECK(t != NULL);
    if (t == NULL) return;
    CHECK(t->root == NULL);
    CHECK(t->size == 0);
    avlDestroy(t);                               /* frees the struct too */

    avlDestroy(NULL);                            /* must not crash */
    avlInit(NULL);

    avlInit(&s);                                 /* tree owned by the caller */
    CHECK(s.root == NULL && s.size == 0);
    avlInsert(&s, 1);
    avlInsert(&s, 2);
    avlInsert(&s, 3);
    avlDestroy(&s);
    CHECK(s.root == NULL && s.size == 0);
    CHECK(avlInsert(&s, 5) == 1 && s.size == 1); /* reusable after destroy */
    avlDestroy(&s);

    t = avlCreate();                             /* destroy with nodes inside */
    avlInsert(t, 1);
    avlInsert(t, 2);
    avlInsert(t, 3);
    avlDestroy(t);                               /* leak check: use -fsanitize=address */
}

/* records arrive out of chronological order, must come out in order */
static void test_insert_out_of_order(void)
{
    AVLTree *t = avlCreate();
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

    CHECK(t != NULL);
    if (t == NULL) return;

    for (i = 0; i < 5; i++)
        CHECK(avlInsert(t, ts(arrival[i])) == 1);

    CHECK(t->size == 5);
    CHECK(avlListAscending(t, out, 8) == 5);
    for (i = 0; i < 5; i++)
        CHECK(out[i] == ts(expected[i]));
    CHECK(is_valid(t));
    CHECK(avlInsert(NULL, 1) == 0);
    avlDestroy(t);
}

/* this tree rejects duplicates: 2nd insert returns 0 and nothing changes */
static void test_insert_duplicate(void)
{
    AVLTree *t = avlCreate();

    CHECK(t != NULL);
    if (t == NULL) return;

    CHECK(avlInsert(t, 50) == 1);
    CHECK(avlInsert(t, 50) == 0);
    CHECK(t->size == 1);
    CHECK(is_valid(t));
    avlDestroy(t);
}

/* the 4 rebalancing cases, each one ends as 10 <- 20 -> 30 */
static void three_keys(time_t a, time_t b, time_t c)
{
    AVLTree *t = avlCreate();

    CHECK(t != NULL);
    if (t == NULL) return;

    avlInsert(t, a);
    avlInsert(t, b);
    avlInsert(t, c);

    CHECK(t->root != NULL);
    if (t->root != NULL) {
        CHECK(t->root->value == 20);
        CHECK(t->root->left != NULL && t->root->left->value == 10);
        CHECK(t->root->right != NULL && t->root->right->value == 30);
        CHECK(t->root->height == 2);
    }
    CHECK(is_valid(t));
    avlDestroy(t);
}

static void test_insert_rotations(void)
{
    three_keys(30, 20, 10);   /* left-left:   single right rotation */
    three_keys(10, 20, 30);   /* right-right: single left rotation  */
    three_keys(30, 10, 20);   /* left-right:  double rotation       */
    three_keys(10, 30, 20);   /* right-left:  double rotation       */
}

/* sorted input is the worst case for a plain BST (it would become a list) */
static void test_insert_sequential_stays_balanced(void)
{
    AVLTree *up = avlCreate();
    AVLTree *down = avlCreate();
    int i, bad = 0;

    CHECK(up != NULL && down != NULL);
    if (up == NULL || down == NULL) {
        avlDestroy(up);
        avlDestroy(down);
        return;
    }

    for (i = 1; i <= 1000; i++) {
        if (avlInsert(up, (time_t)i) != 1) bad++;
        if (avlInsert(down, (time_t)(1001 - i)) != 1) bad++;
    }
    CHECK(bad == 0);
    CHECK(up->size == 1000 && down->size == 1000);
    CHECK(is_valid(up) && is_valid(down));
    CHECK(up->root->height <= height_limit(1000));
    CHECK(down->root->height <= height_limit(1000));
    avlDestroy(up);
    avlDestroy(down);
}

/* values past January 2038 do not fit in a 32-bit int */
static void test_value_is_full_time_t(void)
{
    AVLTree *t = avlCreate();
    time_t out[3];

    CHECK(t != NULL);
    if (t == NULL) return;

    avlInsert(t, ts("2040-01-01 00:00:00"));
    avlInsert(t, ts("2020-01-01 00:00:00"));
    avlInsert(t, ts("2030-06-15 12:30:00"));

    CHECK(avlListAscending(t, out, 3) == 3);
    CHECK(out[0] == ts("2020-01-01 00:00:00"));
    CHECK(out[1] == ts("2030-06-15 12:30:00"));
    CHECK(out[2] == ts("2040-01-01 00:00:00"));
    CHECK(avlRemove(t, ts("2040-01-01 00:00:00")) == 1);
    CHECK(t->size == 2);
    avlDestroy(t);
}

static void test_remove(void)
{
    static const long long after_leaf[]  = { 30, 40, 50, 60, 70, 80 };
    static const long long after_one[]   = { 40, 50, 60, 70, 80 };
    static const long long after_two[]   = { 40, 50, 60, 80 };
    static const long long after_root[]  = { 40, 60, 80 };
    AVLTree *t = avlCreate();
    int i;

    CHECK(t != NULL);
    if (t == NULL) return;

    CHECK(avlRemove(t, 1) == 0);                 /* empty tree */
    CHECK(avlRemove(NULL, 1) == 0);

    for (i = 20; i <= 80; i += 10)               /* 20 30 40 50 60 70 80 */
        avlInsert(t, (time_t)i);                 /* (arrives sorted, rotations happen) */

    CHECK(avlRemove(t, 20) == 1);                /* leaf */
    CHECK(t->size == 6 && is_valid(t) && list_is(t, after_leaf, 6));
    CHECK(avlRemove(t, 30) == 1);                /* node with one child */
    CHECK(t->size == 5 && is_valid(t) && list_is(t, after_one, 5));
    CHECK(avlRemove(t, 70) == 1);                /* node with two children */
    CHECK(t->size == 4 && is_valid(t) && list_is(t, after_two, 4));
    CHECK(avlRemove(t, 50) == 1);                /* whatever is at the root now */
    CHECK(t->size == 3 && is_valid(t) && list_is(t, after_root, 3));

    CHECK(avlRemove(t, 55) == 0);                /* never existed */
    CHECK(avlRemove(t, 5) == 0 && avlRemove(t, 99) == 0);
    CHECK(avlRemove(t, 50) == 0);                /* already removed */
    CHECK(t->size == 3 && is_valid(t));

    CHECK(avlRemove(t, 40) == 1);
    CHECK(avlRemove(t, 60) == 1);
    CHECK(avlRemove(t, 80) == 1);                /* now empty */
    CHECK(t->size == 0 && t->root == NULL);
    CHECK(avlRemove(t, 80) == 0);
    CHECK(avlInsert(t, 7) == 1 && t->size == 1); /* still usable */
    avlDestroy(t);
}

/* removing can unbalance the tree: it must rotate on the way back up */
static void test_remove_rebalances(void)
{
    AVLTree *t = avlCreate();

    CHECK(t != NULL);
    if (t == NULL) return;

    avlInsert(t, 10);
    avlInsert(t, 5);
    avlInsert(t, 15);
    avlInsert(t, 3);                             /* 10 -> (5 -> (3), 15) */

    CHECK(avlRemove(t, 15) == 1);                /* left side becomes too tall */
    CHECK(is_valid(t));
    CHECK(t->root != NULL && t->root->value == 5);
    if (t->root != NULL) {
        CHECK(t->root->left != NULL && t->root->left->value == 3);
        CHECK(t->root->right != NULL && t->root->right->value == 10);
    }
    avlDestroy(t);
}

static void test_locate(void)
{
    AVLTree *t = avlCreate();
    AVLNode *n;

    CHECK(t != NULL);
    if (t == NULL) return;

    avlInsert(t, 10);
    avlInsert(t, 20);
    avlInsert(t, 30);

    n = avlLocate(t, 20);
    CHECK(n != NULL && n->value == 20);
    CHECK(avlLocate(t, 25) == NULL);
    CHECK(avlLocate(NULL, 20) == NULL);

    avlRemove(t, 20);
    CHECK(avlLocate(t, 20) == NULL);             /* gone after removal */
    avlDestroy(t);
}

static void test_list_ascending(void)
{
    AVLTree *t = avlCreate();
    time_t out[8];
    time_t guard[6];
    int i;

    CHECK(t != NULL);
    if (t == NULL) return;

    CHECK(avlListAscending(t, out, 8) == 0);     /* empty tree */

    avlInsert(t, 30);
    avlInsert(t, 10);
    avlInsert(t, 50);
    avlInsert(t, 20);
    avlInsert(t, 40);

    CHECK(avlListAscending(t, out, 8) == 5);     /* capacity bigger than size */
    CHECK(out[0] == 10 && out[1] == 20 && out[2] == 30 && out[3] == 40 && out[4] == 50);

    for (i = 0; i < 6; i++)
        guard[i] = -777;
    CHECK(avlListAscending(t, guard, 3) == 3);   /* capacity smaller than size */
    CHECK(guard[0] == 10 && guard[1] == 20 && guard[2] == 30);
    CHECK(guard[3] == -777 && guard[4] == -777 && guard[5] == -777); /* nothing written past it */

    CHECK(avlListAscending(NULL, out, 8) == 0);
    CHECK(avlListAscending(t, NULL, 8) == 0);
    CHECK(avlListAscending(t, out, 0) == 0);
    avlPrint(NULL);                              /* must not crash */
    avlPrintTreeBrute(NULL);
    avlDestroy(t);
}

/* random insert/remove against a trivial sorted-array model, checking the
 * tree after EVERY operation (this is what catches rebalancing bugs) */
static void test_random_against_model(void)
{
    long long model[512];
    int mn = 0, step, bad = 0;
    AVLTree *t = avlCreate();

    CHECK(t != NULL);
    if (t == NULL) return;

    for (step = 0; step < 4000 && !bad; step++) {
        long long val = (long long)(rnd() % 300);
        int lo = 0, hi = mn, k, present, ret;

        while (lo < hi) {                        /* model lower bound */
            int mid = (lo + hi) / 2;
            if (model[mid] < val) lo = mid + 1;
            else hi = mid;
        }
        present = (lo < mn && model[lo] == val);

        if (rnd() % 3 != 0) {                    /* insert (2/3 of the steps) */
            ret = avlInsert(t, (time_t)val);
            if (ret != !present) bad = 1;
            if (!present) {
                for (k = mn; k > lo; k--)
                    model[k] = model[k - 1];
                model[lo] = val;
                mn++;
            }
        } else {                                 /* remove */
            ret = avlRemove(t, (time_t)val);
            if (ret != present) bad = 1;
            if (present) {
                for (k = lo; k < mn - 1; k++)
                    model[k] = model[k + 1];
                mn--;
            }
        }
        if (!bad && (!is_valid(t) || !list_is(t, model, (size_t)mn))) {
            printf("    (broken after step %d, value %lld)\n", step, val);
            bad = 1;
        }
    }
    CHECK(!bad);
    avlDestroy(t);
}

/* a bigger run, closer to what the benchmark will do */
#define BIG 20000
static void test_large_volume(void)
{
    const long long base = 1700000000LL, step = 37;
    int *order = malloc(BIG * sizeof *order);
    time_t *out = malloc(BIG * sizeof *out);
    AVLTree *t = avlCreate();
    int i, j, bad;

    CHECK(order != NULL && out != NULL && t != NULL);
    if (order == NULL || out == NULL || t == NULL) {
        free(order); free(out); avlDestroy(t);
        return;
    }

    for (i = 0; i < BIG; i++)                    /* shuffled arrival order */
        order[i] = i;
    for (i = BIG - 1; i > 0; i--) {
        int tmp;
        j = (int)(rnd() % (unsigned)(i + 1));
        tmp = order[i]; order[i] = order[j]; order[j] = tmp;
    }

    for (i = 0, bad = 0; i < BIG; i++)
        if (avlInsert(t, (time_t)(base + order[i] * step)) != 1)
            bad++;
    CHECK(bad == 0 && t->size == BIG && is_valid(t));
    CHECK(t->root->height <= height_limit(BIG));

    CHECK(avlListAscending(t, out, BIG) == BIG);
    for (i = 0, bad = 0; i < BIG; i++)
        if (out[i] != (time_t)(base + i * step))
            bad++;
    CHECK(bad == 0);

    for (i = 0, bad = 0; i < BIG; i++)           /* remove the even ones */
        if (order[i] % 2 == 0 && avlRemove(t, (time_t)(base + order[i] * step)) != 1)
            bad++;
    CHECK(bad == 0 && t->size == BIG / 2 && is_valid(t));

    CHECK(avlListAscending(t, out, BIG) == BIG / 2);
    for (i = 0, bad = 0; i < BIG / 2; i++)       /* only the odd ones remain */
        if (out[i] != (time_t)(base + (2 * i + 1) * step))
            bad++;
    CHECK(bad == 0);

    free(order);
    free(out);
    avlDestroy(t);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);            /* output survives a crash */
    puts("avl tree tests\n");

    RUN(test_create_destroy);
    RUN(test_insert_out_of_order);
    RUN(test_insert_duplicate);
    RUN(test_insert_rotations);
    RUN(test_insert_sequential_stays_balanced);
    RUN(test_value_is_full_time_t);
    RUN(test_remove);
    RUN(test_remove_rebalances);
    RUN(test_locate);
    RUN(test_list_ascending);
    RUN(test_random_against_model);
    RUN(test_large_volume);

    printf("\n%d checks, %d failed\n", checks, failed);
    return failed ? 1 : 0;
}