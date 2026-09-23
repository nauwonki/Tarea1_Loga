#include "binomial.h"
#include "prim.h"

#include <math.h>
#include <stdlib.h>
#include <float.h>

PrimResult primBinomial(const Graph *g, int root, int *parentOut, DecreaseLog *log) {
    PrimResult result;
    double *key;
    BinomialNode **nodeOf;
    BinomialHeap *heap;
    int *parent;
    int i;
    result.mstWeight = 0.0;
    result.mstEdges = 0;
    result.totalTime = 0.0;
    result.extractMinCalls = 0;
    result.decreaseKeyCalls = 0;
    result.decreaseKeyOps = 0;
    result.decreaseKeyTime = 0.0;

    if (g == NULL || g->V <=0) {
        return result;
    }
    if (root < 0 || root >= g->V) {
        return result;
    }
    
    key = malloc((size_t)g->V * sizeof(double));
    /* nodeOf[v] apunta al nodo que representa actualmente al vértice v */
    nodeOf = malloc((size_t)g->V * sizeof(BinomialNode *));
    /* parent[v] almacena al padre de v */
    parent = malloc((size_t)g->V * sizeof(int));

    if (key == NULL || nodeOf == NULL || parent == NULL) {
        free(key);
        free(nodeOf);
        free(parent);
        return result;
    }

    /* Inicializar Prim */
    for (i = 0; i < g->V; i++) {
        key[i] = DBL_MAX;
        nodeOf[i] = NULL;
        parent[i] = -1;
    }
    key[root] = 0.0;
    {
        double start = nowSeconds();
        /* Constuir cola con los vértices */
        heap = buildBinomialHeap(key, g->V, nodeOf);
        if (heap == NULL) {
            free(key);
            free(nodeOf);
            free(parent);
            return result;
        }
        while (heap->head != NULL) {
            BinomialNode *uNode;
            int u;
            /* Extraer vértice de menor key */
            uNode = extractMinBinomialHeap(heap);

            if (uNode == NULL) {
                break;
            }
            result.extractMinCalls++;
            u = uNode->vertex;
            
            nodeOf[u] = NULL;
            result.mstWeight += uNode->key;
            if (u != root) {
                result.mstEdges++;
            }
            
            {
                /* Revisar vecinos de u */
                Edge *edge = g->adj[u];
                while (edge != NULL) {
                    int v = edge->to;
                    if (nodeOf[v] != NULL && edge->weight < nodeOf[v]->key) {
                        double startDecrease;
                        double endDecrease;
                        long long swapsBefore;
                        parent[v] = u;
                        /* Contabilizar decreaseKey */
                        result.decreaseKeyCalls++;
                        swapsBefore = result.decreaseKeyOps;
                        if (log != NULL) {
                            startDecrease = nowSeconds();
                            decreaseKeyBinomialHeap(heap, nodeOf[v], edge->weight, nodeOf, &result.decreaseKeyOps);
                            endDecrease = nowSeconds();
                            result.decreaseKeyTime += (endDecrease - startDecrease);
                            decreaseLogPush(log, endDecrease - startDecrease, result.decreaseKeyOps - swapsBefore);
                        } else {
                            decreaseKeyBinomialHeap(heap, nodeOf[v], edge->weight, nodeOf, &result.decreaseKeyOps);
                        }
                        key[v] = edge->weight;
                    }
                    edge = edge->next;
                }
            }
            free(uNode);
        }
        result.totalTime = nowSeconds() - start;
    }
    if (parentOut != NULL) {
        for (i = 0; i < g->V; i++) {
            parentOut[i] = parent[i];
        }
    }
    deleteBinomialHeap(heap);
    free(key);
    free(nodeOf);
    free(parent);
    return result;
}