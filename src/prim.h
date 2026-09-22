#ifndef PRIM_H
#define PRIM_H
#include "graph.h"

typedef struct {
    long long decrease;
    long long operations;
    double decrease_time;
} DecreaseStats;

typedef struct {
    double mst_weight;
    double total_time;
    DecreaseStats decrease_stats;
} PrimResult;

PrimResult primBinomial(
     const Graph *g 
);

PrimResult primFibonacci(
     const Graph *g 
);

#endif