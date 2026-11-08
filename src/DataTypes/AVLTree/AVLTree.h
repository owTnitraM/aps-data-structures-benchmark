#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <stddef.h>
#include <time.h>

typedef struct AVLNode {
    time_t value;
    int height;

    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

typedef struct AVLTree {
    AVLNode *root;
    size_t size;
} AVLTree;

AVLTree *avlCreate(void);
void avlDestroy(AVLTree *tree);

int avlInsert(AVLTree *tree, time_t value);
AVLNode *avlLocate(const AVLTree *tree, time_t value);
int avlRemove(AVLTree *tree, time_t value);
size_t avlListAscending(const AVLTree *tree, time_t *out, size_t capacity);

void avlPrintNode(const AVLNode *node);
void avlPrintBrute(const AVLNode *node);
void avlPrint(const AVLTree *tree);
void avlPrintTreeBrute(const AVLTree *tree);

#endif
