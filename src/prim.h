#ifndef PRIM_H
#define PRIM_H

#include "graph.h"
#include "measure.h"

/* Resultado de una corrida del algoritmo de Prim. */
typedef struct {
    double mstWeight;           /* suma de los pesos del arbol cobertor minimo */
    long long mstEdges;         /* aristas del MST; debe ser V-1 si el grafo es conexo */
    double totalTime;           /* tiempo del algoritmo en segundos, sin contar la
                                 * generacion del grafo ni la lectura de datos */
    long long extractMinCalls;  /* llamadas a extractMin */
    long long decreaseKeyCalls; /* llamadas a decreaseKey */
    long long decreaseKeyOps;   /* total de operaciones estructurales de decreaseKey */
    double decreaseKeyTime;     /* tiempo sumado de las llamadas a decreaseKey;
                                 * 0 si la corrida fue sin bitacora */
} PrimResult;

/* ---------------------------------------------------------------------------
 * Contrato comun de las dos versiones de Prim.
 *
 * Ambas construyen la cola inicial con los V costos (infinito salvo la raiz)
 * mediante inserciones sucesivas, mantienen un arreglo de punteros a los nodos
 * de Q para poder hacer decreaseKey en tiempo acotado, y marcan un vertice
 * como fuera de la cola poniendo su puntero en NULL al extraerlo (asi el test
 * "u esta en Q" del pseudocodigo no necesita un arreglo extra).
 *
 * Parametros:
 *   g         grafo conexo de entrada; no se modifica.
 *   root      vertice de partida, en [0, g->V-1].
 *   parentOut si no es NULL, arreglo de al menos g->V enteros donde se deja el
 *             MST como parent[v] = predecesor de v, con parent[root] = -1.
 *   log       si no es NULL, se cronometra cada llamada a decreaseKey y se
 *             registra en la bitacora. Debe ser NULL en el experimento de
 *             tiempo total para no distorsionar la medicion.
 *
 * Retorna las metricas de la corrida. Si el grafo no es conexo o los
 * parametros son invalidos, retorna un PrimResult con mstEdges < g->V-1. */

/* Prim usando una cola binomial: O(e log v) en el peor caso. */
PrimResult primBinomial(
    const Graph *g,
    int root,
    int *parentOut,
    DecreaseLog *log
);

/* Prim usando una cola de Fibonacci: O(e + v log v) amortizado. */
PrimResult primFibonacci(
    const Graph *g,
    int root,
    int *parentOut,
    DecreaseLog *log
);

#endif
