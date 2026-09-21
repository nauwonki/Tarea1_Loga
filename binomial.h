#ifndef BINOMIAL_H
#define BINOMIAL_H

typedef struct BinomialNode {
    int vertex;
    double key;
    int degree;
    struct BinomialNode* parent;
    struct BinomialNode* child;
    struct BinomialNode* sibling;
} BinomialNode;

typedef struct {
    BinomialNode* head;
    int size;
} BinomialHeap;

BinomialHeap *createBinomialHeap(void);

BinomialNode *createBinomialNode(
    int vertex, 
    double key
);

void insertBinomialHeap(
    BinomialHeap *h, 
    BinomialNode *node
);

BinomialNode *extractMinBinomialHeap(
    BinomialHeap *h
);

void decreaseKeyBinomialHeap(
    BinomialNode *x,
    double newKey,
    BinomialNode **Q,
    long long *operations
);

void deleteBinomialHeap(
    BinomialHeap *h
);

#endif