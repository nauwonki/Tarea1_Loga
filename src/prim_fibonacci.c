#include "fibonacci.h"
#include "prim.h"

#include <math.h>
#include <stdlib.h>

PrimResult primFibonacci(const Graph *g, int root, int *parentOut, DecreaseLog *log) {
    PrimResult result;
    result.mstWeight = 0.0;
    result.mstEdges = -1; /* queda en -1 si la corrida no llega a terminar */
    result.totalTime = 0.0;
    result.extractMinCalls = 0;
    result.decreaseKeyCalls = 0;
    result.decreaseKeyOps = 0;
    result.decreaseKeyTime = 0.0;

    if (g == NULL || root < 0 || root >= g->V) {
        return result;
    }

    int V = g->V;
    double *costs = malloc((size_t)V * sizeof *costs);
    int *parent = (parentOut != NULL) ? parentOut : malloc((size_t)V * sizeof *parent);
    FibonacciNode **nodeOf = malloc((size_t)V * sizeof *nodeOf);

    if (costs == NULL || parent == NULL || nodeOf == NULL) {
        free(costs);
        if (parent != parentOut) {
            free(parent);
        }
        free(nodeOf);
        return result;
    }

    /* El cronometro arranca en la linea 1 del pseudocodigo: inicializar los
     * costos y construir Q son parte del algoritmo, mientras que reservar los
     * arreglos es preparacion del experimento y queda fuera. */
    double start = nowSeconds();

    for (int v = 0; v < V; v++) {
        costs[v] = INFINITY;
        parent[v] = -1;
    }
    costs[root] = 0.0;

    FibonacciHeap *h = buildFibonacciHeap(costs, V, nodeOf);
    if (h == NULL) {
        free(costs);
        if (parent != parentOut) {
            free(parent);
        }
        free(nodeOf);
        return result;
    }

    double weight = 0.0;
    long long edges = 0;
    long long calls = 0;
    long long ops = 0;
    double decreaseTime = 0.0;

    while (h->n > 0) {
        FibonacciNode *z = extractMinFibonacciHeap(h);
        int v = z->vertex;
        double cost = z->key;
        free(z);

        /* Poner el puntero en NULL es lo que marca que v ya salio de Q, asi el
         * test "u esta en Q" del pseudocodigo no necesita un arreglo extra. */
        nodeOf[v] = NULL;
        result.extractMinCalls++;

        /* Un costo infinito significa que ninguna arista conecta v con el arbol
         * construido hasta ahora: el grafo no es conexo y v abre una nueva
         * componente, por lo que no aporta arista al resultado. */
        if (v != root && isfinite(cost)) {
            weight += cost;
            edges++;
        }

        for (const Edge *e = g->adj[v]; e != NULL; e = e->next) {
            int u = e->to;
            if (nodeOf[u] == NULL || !(e->weight < costs[u])) {
                continue;
            }

            costs[u] = e->weight;
            parent[u] = v;
            calls++;

            if (log != NULL) {
                long long before = ops;
                double t0 = nowSeconds();
                decreaseKeyFibonacciHeap(h, nodeOf[u], e->weight, &ops);
                double t1 = nowSeconds();
                decreaseTime += t1 - t0;
                decreaseLogPush(log, t1 - t0, ops - before);
            } else {
                decreaseKeyFibonacciHeap(h, nodeOf[u], e->weight, &ops);
            }
        }
    }

    double end = nowSeconds();

    result.mstWeight = weight;
    result.mstEdges = edges;
    result.totalTime = end - start;
    result.decreaseKeyCalls = calls;
    result.decreaseKeyOps = ops;
    result.decreaseKeyTime = decreaseTime;

    deleteFibonacciHeap(h);
    free(costs);
    if (parent != parentOut) {
        free(parent);
    }
    free(nodeOf);
    return result;
}
