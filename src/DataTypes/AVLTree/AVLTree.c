#include "AVLTree.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int height(const AVLNode *node) {
    return node == NULL ? 0 : node->height;
}

static int max(int a, int b) {
    return a > b ? a : b;
}

static void updateHeight(AVLNode *node) {
    if (node == NULL)
        return;

    node->height = 1 + max(
        height(node->left),
        height(node->right)
    );
}

static int balanceFactor(const AVLNode *node) {
    if (node == NULL)
        return 0;

    return height(node->left) - height(node->right);
}

static AVLNode *rotateLeft(AVLNode *node) {
    AVLNode *newRoot = node->right;
    AVLNode *temp = newRoot->left;

    newRoot->left = node;
    node->right = temp;

    updateHeight(node);
    updateHeight(newRoot);

    return newRoot;
}

static AVLNode *rotateRight(AVLNode *node) {
    AVLNode *newRoot = node->left;
    AVLNode *temp = newRoot->right;

    newRoot->right = node;
    node->left = temp;

    updateHeight(node);
    updateHeight(newRoot);

    return newRoot;
}

static AVLNode *rebalance(AVLNode *node) {
    if (node == NULL)
        return NULL;

    updateHeight(node);

    int balance = balanceFactor(node);

    // Desbalanceamento à esquerda
    if (balance > 1) {
        if (balanceFactor(node->left) < 0)
            node->left = rotateLeft(node->left);

        return rotateRight(node);
    }

    // Desbalanceamento à direita
    if (balance < -1) {
        if (balanceFactor(node->right) > 0)
            node->right = rotateRight(node->right);

        return rotateLeft(node);
    }

    return node;
}

static AVLNode *createNode(time_t value) {
    AVLNode *node = malloc(sizeof(AVLNode));

    if (node == NULL)
        return NULL;

    node->value = value;
    node->height = 1;
    node->left = NULL;
    node->right = NULL;

    return node;
}

void avlInit(AVLTree *tree) {
    if (tree == NULL)
        return;

    tree->root = NULL;
}

static AVLNode *insertNode(
    AVLNode *node,
    time_t value,
    int *inserted
) {
    if (node == NULL) {
        AVLNode *newNode = createNode(value);

        if (newNode != NULL)
            *inserted = 1;

        return newNode;
    }

    if (value < node->value) {
        node->left = insertNode(
            node->left,
            value,
            inserted
        );
    }
    else if (value > node->value) {
        node->right = insertNode(
            node->right,
            value,
            inserted
        );
    }
    else {
        // Valores duplicados não são inseridos
        return node;
    }

    return rebalance(node);
}

int avlInsert(AVLTree *tree, time_t value) {
    if (tree == NULL)
        return 0;

    int inserted = 0;

    tree->root = insertNode(
        tree->root,
        value,
        &inserted
    );

    return inserted;
}

AVLNode *avlLocate(const AVLTree *tree, time_t value) {
    if (tree == NULL)
        return NULL;

    AVLNode *current = tree->root;

    while (current != NULL) {
        if (value == current->value)
            return current;

        if (value < current->value)
            current = current->left;
        else
            current = current->right;
    }

    return NULL;
}

static AVLNode *findMinNode(AVLNode *node) {
    while (node != NULL && node->left != NULL)
        node = node->left;

    return node;
}

static AVLNode *removeNode(
    AVLNode *node,
    time_t value,
    int *removed
) {
    if (node == NULL)
        return NULL;

    if (value < node->value) {
        node->left = removeNode(
            node->left,
            value,
            removed
        );
    }
    else if (value > node->value) {
        node->right = removeNode(
            node->right,
            value,
            removed
        );
    }
    else {
        *removed = 1;

        // Nó sem filhos ou com apenas um filho
        if (node->left == NULL || node->right == NULL) {
            AVLNode *child = node->left != NULL
                ? node->left
                : node->right;

            free(node);
            return child;
        }

        // Nó com dois filhos
        AVLNode *successor = findMinNode(node->right);

        node->value = successor->value;

        int successorRemoved = 0;

        node->right = removeNode(
            node->right,
            successor->value,
            &successorRemoved
        );
    }

    return rebalance(node);
}

int avlRemove(AVLTree *tree, time_t value) {
    if (tree == NULL)
        return 0;

    int removed = 0;

    tree->root = removeNode(
        tree->root,
        value,
        &removed
    );

    return removed;
}

static void destroyNode(AVLNode *node) {
    if (node == NULL)
        return;

    destroyNode(node->left);
    destroyNode(node->right);

    free(node);
}

void avlDestroy(AVLTree *tree) {
    if (tree == NULL)
        return;

    destroyNode(tree->root);
    tree->root = NULL;
}

void avlPrintNode(const AVLNode *node) {
    if (node == NULL) {
        printf("Node nao encontrado.\n");
        return;
    }

    struct tm *date = localtime(&node->value);

    if (date == NULL) {
        printf("Data invalida");
        return;
    }

    char buffer[32];

    if (strftime(
        buffer,
        sizeof(buffer),
        "%d/%m/%Y %H:%M:%S",
        date
    ) == 0) {
        printf("Data invalida");
        return;
    }

    printf("%s", buffer);
}

void avlPrintBrute(const AVLNode *node) {
    if (node == NULL) {
        printf("Node nao encontrado.\n");
        return;
    }

    printf("%lld", (long long)node->value);
}

static void printPrefix(
    const int *branches,
    int depth
) {
    for (int i = 0; i < depth; i++) {
        printf(branches[i] ? "│   " : "    ");
    }
}

static void printTreeNode(
    const AVLNode *node,
    int depth,
    int isLast,
    int *branches,
    int brute
) {
    if (node == NULL)
        return;

    printPrefix(branches, depth);

    printf(isLast ? "└── " : "├── ");

    if (brute)
        avlPrintBrute(node);
    else
        avlPrintNode(node);

    printf(
        " [h=%d, FB=%d]\n",
        node->height,
        balanceFactor(node)
    );

    const AVLNode *children[2];
    int count = 0;

    if (node->left != NULL)
        children[count++] = node->left;

    if (node->right != NULL)
        children[count++] = node->right;

    branches[depth] = !isLast;

    for (int i = 0; i < count; i++) {
        printTreeNode(
            children[i],
            depth + 1,
            i == count - 1,
            branches,
            brute
        );
    }
}

static void printTree(const AVLTree *tree, int brute) {
    if (tree == NULL)
        return;

    if (tree->root == NULL) {
        printf("Arvore vazia.\n");
        return;
    }

    printf(
        brute
            ? "\n========== ARVORE AVL BRUTA ==========\n\n"
            : "\n========== ARVORE AVL ==========\n\n"
    );

    int *branches = calloc(
        (size_t)tree->root->height + 1,
        sizeof(int)
    );

    if (branches == NULL) {
        printf("Erro ao alocar memoria para impressao.\n");
        return;
    }

    if (brute)
        avlPrintBrute(tree->root);
    else
        avlPrintNode(tree->root);

    printf(
        " [h=%d, FB=%d] (RAIZ)\n",
        tree->root->height,
        balanceFactor(tree->root)
    );

    const AVLNode *children[2];
    int count = 0;

    if (tree->root->left != NULL)
        children[count++] = tree->root->left;

    if (tree->root->right != NULL)
        children[count++] = tree->root->right;

    for (int i = 0; i < count; i++) {
        printTreeNode(
            children[i],
            0,
            i == count - 1,
            branches,
            brute
        );
    }

    free(branches);

    printf("\n======================================\n");
}

void avlPrint(const AVLTree *tree) {
    printTree(tree, 0);
}

void avlPrintTreeBrute(const AVLTree *tree) {
    printTree(tree, 1);
}
