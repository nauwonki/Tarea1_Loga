#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>

typedef struct Edge {
    int to;
    double weight;
    struct Edge* next;
} Edge;

typedef struct {
    int V;
    long long E;
    Edge** adj;
} Graph;

Graph* createGraph(int V);

void addEdge(
    Graph *g,
    int u,
    int v,
    double weight
);

Graph *generateGraph(
    int V,
    long long E,
    unsigned int seed
);

int saveGraph(
    const Graph *g,
    const char *filename
);

Graph *loadGraph(
    const char *filename
);

void freeGraph(Graph *g);

#endif
