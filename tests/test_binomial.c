/* Pruebas de la cola de Fibonacci y de Prim sobre ella.
 * Se compila y ejecuta con `make test`. */
#include "binomial.h"
#include "graph.h"
#include "prim.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

static int checks = 0;
static int failures = 0;

#define CHECK(cond, ...)                              \
    do {                                              \
        checks++;                                     \
        if (!(cond)) {                                \
            failures++;                               \
            printf("  FALLA (linea %d): ", __LINE__); \
            printf(__VA_ARGS__);                      \
            printf("\n");                             \
        }                                             \
    } while (0)

/* Pruebas de la Estructura */
static void testExtractOrder(void) {
    printf("Orden de extraccion\n");

    const int n = 1000;
    double *keys = malloc((size_t)n * sizeof(double));
    BinomialNode **nodeOf = malloc((size_t)n * sizeof(BinomialNode *));

    CHECK(keys != NULL, "sin memoria para keys");
    CHECK(nodeOf != NULL, "sin memoria para nodeOf");

    if (keys == NULL || nodeOf == NULL) {
        free(keys);
        free(nodeOf);
        return;
    }
    
    /* keys desordenados */
    for (int i = 0; i < n; i++) {
        keys[i] = (double)(n-1);
    }

    BinomialHeap *h = buildBinomialHeap(keys, n, nodeOf);
    CHECK(h != NULL, "buildBinomialHeap retorno NULL");
    if (h == NULL) {
        free(keys);
        free(nodeOf);
        return;
    }

    double previous = -1.0;
    int extracted = 0;
    int outOfOrder = 0;
    while (h->head != NULL) {
        BinomialNode *node = extractMinBinomialHeap(h);
        CHECK(node != NULL, "extractMinBinomialHeap retorno NULL");
        if (node == NULL) {
            break;
        }
        if (node->key < previous) {
            outOfOrder++;
        }
        previous = node->key;
        extracted++;
        free(node);
    }

    CHECK(extracted == n, "se extrajeron %d nodos de %d", extracted, n);
    CHECK(outOfOrder == 0, "%d extracciones salieron fuera de orden", outOfOrder);
    CHECK(extractMinBinomialHeap(h) == NULL, "una cola vacia devolvio un nodo");
    deleteBinomialHeap(h);

    free(keys);
    free(nodeOf);
}

static void testDecreaseKey(void) {
    printf("Decrease key\n");

    const int n = 100;
    double *keys = malloc((size_t)n * sizeof(double));
    BinomialNode **nodeOf = malloc((size_t)n * sizeof(BinomialNode *));

    if (keys == NULL || nodeOf == NULL) {
        free(keys);
        free(nodeOf);
        return;
    }
    for (int i = 0; i < n; i++) {
        keys[i] = 1000.0 + i;
    }

    BinomialHeap *h = buildBinomialHeap(keys, n, nodeOf);
    CHECK(h != NULL, "buildBinomialHeap retorno NULL");

    if (h == NULL) {
        free(keys);
        free(nodeOf);
        return;
    }
    /* Bajamos la prioridad de un vértice */
    int vertex = 50;
    long long swaps = 0;
    decreaseKeyBinomialHeap(h, nodeOf[vertex], 0.0, nodeOf, &swaps);
    CHECK(nodeOf[vertex] != NULL, "nodeOf[%d] es NULL despues de decreaseKey", vertex);

    /* Extraemos el mínimo y verificamos que sea el vértice que bajamos */
    BinomialNode *minNode = extractMinBinomialHeap(h);
    CHECK(minNode != NULL, "extractMinBinomialHeap retorno NULL");
    if (minNode != NULL) {
        CHECK(minNode->vertex == vertex, "el vertice extraido es %d y deberia ser %d", vertex, minNode->vertex);
        CHECK(minNode->key == 0.0, "la clave del vertice extraido es %f y deberia ser 0.0", minNode->key);
        free(minNode);
    }
    deleteBinomialHeap(h);
    free(keys);
    free(nodeOf);
}

/* Pruebas de Prim */
static void testPrim(void) {
    printf("Prim con cola binomial\n");
    
    Graph *g = createGraph(4, 5);
    CHECK(g != NULL, "fallo la generacion del grafo");
    if (g == NULL) {
        return;
    }

    addEdge(g, 0, 1, 1.0);
    addEdge(g, 1, 2, 2.0);
    addEdge(g, 2, 3, 1.0);
    addEdge(g, 0, 3, 4.0);
    addEdge(g, 0, 2, 5.0);
    int parent[4];

    PrimResult result = primBinomial(g, 0, parent, NULL);
    CHECK(result.mstEdges == 3, "el MST tiene %lld aristas y deberia tener 3", result.mstEdges);
    CHECK(fabs(result.mstWeight - 4.0) < 1e-12, "el peso del MST es %.12f y deberia ser 4.0", result.mstWeight);
    CHECK(result.extractMinCalls == 4, "hubo %lld extractMin y deberian ser 4", result.extractMinCalls);
    freeGraph(g);
}

static void testPrimRandom(void) {
    printf("Prim sobre grafos aleatorios\n");
    
    struct {
        int V;
        long long E;
    } cases[] = {
        {2, 1},
        {16, 20},
        {64, 128},
        {256, 512},
        {1024, 4096},
    };
    int numCases = sizeof(cases) / sizeof(cases[0]);
    for (int k = 0; k < numCases; k++) {
        int V = cases[k].V;
        long long E = cases[k].E;
        Graph *g = generateGraph(V, E, (unsigned int)(100 + k));
        CHECK(g != NULL, "V=%d, E=%lld: fallo la generacion del grafo", V, E);
        if (g == NULL) {
            continue;
        }

        int *parent = malloc((size_t)V * sizeof(int));
        CHECK(parent != NULL, "V=%d: sin memoria para parent", V);
        if (parent == NULL) {
            freeGraph(g);
            continue;
        }
        PrimResult result = primBinomial(g, 0, parent, NULL);
        CHECK(result.mstEdges == V - 1, "V=%d: el MST tiene %lld aristas y deberia tener %d", V, result.mstEdges, V - 1);
        CHECK(result.extractMinCalls == V, "V=%d: hubo %lld extractMin y deberian ser %d", V, result.extractMinCalls, V);

        free(parent);
        freeGraph(g);
    }
}

int main(void) {
    testExtractOrder();
    testDecreaseKey();
    testPrim();
    testPrimRandom();

    printf("Se hicieron %d pruebas y hubo %d fallas\n", checks, failures);
    return failures == 0 ? 0 : 1;
}

