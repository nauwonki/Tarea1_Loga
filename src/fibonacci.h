#ifndef FIBONACCI_H
#define FIBONACCI_H

typedef struct FibonacciNode {
    int vertex;
    double key;
    int degree;
    int marked;
    struct FibonacciNode* parent;
    struct FibonacciNode* child;
    struct FibonacciNode* left;
    struct FibonacciNode* right;
} FibonacciNode;

typedef struct {
    FibonacciNode* min;
    int n;
} FibonacciHeap;

FibonacciHeap *createFibonacciHeap(void);

FibonacciNode *createFibonacciNode(
    int vertex, 
    double key
);

void insertFibonacciHeap(
    FibonacciHeap *h, 
    FibonacciNode *x
);

FibonacciNode *extractMinFibonacciHeap(
    FibonacciHeap *h
);

void decreaseKeyFibonacciHeap(
    FibonacciHeap *h,
    FibonacciNode *x,
    double newKey,
    long long *cuts
);

void deleteFibonacciHeap(
    FibonacciHeap *h
);

#endif
