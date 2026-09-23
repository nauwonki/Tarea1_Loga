/* Pruebas de la cola de Fibonacci y de Prim sobre ella.
 * Se compila y ejecuta con `make test`. */

#include "fibonacci.h"
#include "graph.h"
#include "prim.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

/* Generador simple y reproducible, solo para las pruebas. */
static uint64_t rngState = 0x2545F4914F6CDD1DULL;

static double randomKey(void) {
    rngState += 0x9E3779B97F4A7C15ULL;
    uint64_t z = rngState;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z ^= z >> 31;
    return (double)((z >> 11) + 1) / 9007199254740992.0;
}

/* --- Pruebas de la estructura -------------------------------------------- */

static void testExtractOrder(void) {
    printf("Orden de extraccion\n");

    const int n = 5000;
    double *keys = malloc((size_t)n * sizeof *keys);
    FibonacciNode **nodeOf = malloc((size_t)n * sizeof *nodeOf);
    if (keys == NULL || nodeOf == NULL) {
        free(keys);
        free(nodeOf);
        CHECK(0, "sin memoria");
        return;
    }
    for (int v = 0; v < n; v++) {
        keys[v] = randomKey();
    }

    FibonacciHeap *h = buildFibonacciHeap(keys, n, nodeOf);
    CHECK(h != NULL, "buildFibonacciHeap retorno NULL");
    if (h == NULL) {
        free(keys);
        free(nodeOf);
        return;
    }
    CHECK(h->n == n, "la cola tiene %d nodos y se insertaron %d", h->n, n);

    int extracted = 0;
    int outOfOrder = 0;
    int wrongKey = 0;
    double previous = -1.0;
    while (h->n > 0) {
        FibonacciNode *z = extractMinFibonacciHeap(h);
        if (z == NULL) {
            break;
        }
        if (z->key < previous) {
            outOfOrder++;
        }
        /* El nodo extraido debe seguir representando al vertice con su clave. */
        if (z->vertex < 0 || z->vertex >= n || keys[z->vertex] != z->key) {
            wrongKey++;
        }
        previous = z->key;
        extracted++;
        free(z);
    }

    CHECK(extracted == n, "se extrajeron %d nodos de %d", extracted, n);
    CHECK(outOfOrder == 0, "%d extracciones salieron fuera de orden", outOfOrder);
    CHECK(wrongKey == 0, "%d nodos extraidos no coinciden con su clave original", wrongKey);
    CHECK(extractMinFibonacciHeap(h) == NULL, "una cola vacia devolvio un nodo");

    deleteFibonacciHeap(h);
    free(keys);
    free(nodeOf);
}

static void testDecreaseKey(void) {
    printf("decreaseKey y cortes en cascada\n");

    const int n = 4096;
    double *keys = malloc((size_t)n * sizeof *keys);
    FibonacciNode **nodeOf = malloc((size_t)n * sizeof *nodeOf);
    if (keys == NULL || nodeOf == NULL) {
        free(keys);
        free(nodeOf);
        CHECK(0, "sin memoria");
        return;
    }
    for (int v = 0; v < n; v++) {
        keys[v] = 1.0;
    }

    FibonacciHeap *h = buildFibonacciHeap(keys, n, nodeOf);
    if (h == NULL) {
        free(keys);
        free(nodeOf);
        CHECK(0, "buildFibonacciHeap retorno NULL");
        return;
    }

    /* Una extraccion fuerza la consolidacion, que es lo que arma arboles
     * profundos; sin eso todos los nodos serian raices y nunca habria cortes. */
    FibonacciNode *first = extractMinFibonacciHeap(h);
    free(first);
    nodeOf[0] = NULL;

    long long totalCuts = 0;
    int cascades = 0;
    int decreases = 0;
    for (int v = n - 1; v >= 1; v--) {
        if (nodeOf[v] == NULL) {
            continue;
        }
        long long before = totalCuts;
        double newKey = 0.5 - (double)v / (2.0 * n);
        decreaseKeyFibonacciHeap(h, nodeOf[v], newKey, &totalCuts);
        keys[v] = newKey;
        decreases++;
        if (totalCuts - before >= 2) {
            cascades++;
        }
    }

    CHECK(decreases > 0, "no se hizo ninguna llamada a decreaseKey");
    CHECK(totalCuts > 0, "ninguna llamada a decreaseKey corto un nodo");
    CHECK(cascades > 0, "ninguna llamada produjo un corte en cascada");

    /* Aumentar una clave debe ser un no-op. */
    int probe = n / 2;
    if (nodeOf[probe] != NULL) {
        double keep = nodeOf[probe]->key;
        decreaseKeyFibonacciHeap(h, nodeOf[probe], keep + 1.0, NULL);
        CHECK(nodeOf[probe]->key == keep, "decreaseKey acepto aumentar la clave");
    }

    /* Despues de todos los cortes la cola debe seguir entregando las claves en
     * orden y sin perder ni duplicar nodos. */
    int extracted = 0;
    int outOfOrder = 0;
    double previous = -1.0;
    while (h->n > 0) {
        FibonacciNode *z = extractMinFibonacciHeap(h);
        if (z == NULL) {
            break;
        }
        if (z->key < previous) {
            outOfOrder++;
        }
        previous = z->key;
        extracted++;
        free(z);
    }
    CHECK(extracted == n - 1, "se extrajeron %d nodos y quedaban %d", extracted, n - 1);
    CHECK(outOfOrder == 0, "%d extracciones salieron fuera de orden tras los cortes", outOfOrder);

    deleteFibonacciHeap(h);
    free(keys);
    free(nodeOf);
}

static void testFreeNonEmpty(void) {
    printf("Liberacion de una cola no vacia\n");

    const int n = 1000;
    double *keys = malloc((size_t)n * sizeof *keys);
    FibonacciNode **nodeOf = malloc((size_t)n * sizeof *nodeOf);
    if (keys == NULL || nodeOf == NULL) {
        free(keys);
        free(nodeOf);
        CHECK(0, "sin memoria");
        return;
    }
    for (int v = 0; v < n; v++) {
        keys[v] = randomKey();
    }

    FibonacciHeap *h = buildFibonacciHeap(keys, n, nodeOf);
    /* Extraer algunos nodos deja arboles con hijos, que es el caso que tiene
     * que recorrer deleteFibonacciHeap. */
    for (int k = 0; k < 10 && h != NULL && h->n > 0; k++) {
        free(extractMinFibonacciHeap(h));
    }
    CHECK(h != NULL && h->n == n - 10, "la cola quedo con una cantidad inesperada de nodos");
    deleteFibonacciHeap(h);
    deleteFibonacciHeap(NULL);

    free(keys);
    free(nodeOf);
}

/* --- Pruebas de Prim ----------------------------------------------------- */

/* Peso de la arista (u,v) en el grafo, o -1 si no existe. */
static double edgeWeight(const Graph *g, int u, int v) {
    for (const Edge *e = g->adj[u]; e != NULL; e = e->next) {
        if (e->to == v) {
            return e->weight;
        }
    }
    return -1.0;
}

/* Verifica que parent describa un arbol cobertor del grafo con raiz en root y
 * que su peso coincida con el reportado. Retorna 1 si todo cuadra. */
static int validateTree(const Graph *g, int root, const int *parent, double reportedWeight) {
    int V = g->V;
    double sum = 0.0;
    int edges = 0;
    int missing = 0;

    for (int v = 0; v < V; v++) {
        if (v == root) {
            if (parent[v] != -1) {
                missing++;
            }
            continue;
        }
        if (parent[v] < 0 || parent[v] >= V) {
            missing++;
            continue;
        }
        double w = edgeWeight(g, parent[v], v);
        if (w < 0.0) {
            missing++;
            continue;
        }
        sum += w;
        edges++;
    }

    if (missing != 0 || edges != V - 1) {
        return 0;
    }

    /* Siguiendo los padres desde cualquier vertice se debe llegar a la raiz, lo
     * que descarta ciclos y componentes sueltas. */
    for (int v = 0; v < V; v++) {
        int steps = 0;
        int cur = v;
        while (cur != root && steps <= V) {
            cur = parent[cur];
            steps++;
        }
        if (cur != root) {
            return 0;
        }
    }

    return fabs(sum - reportedWeight) < 1e-9;
}

#define SMALL10_MST_WEIGHT 2.25

static void testPrimSmall(void) {
    printf("Prim sobre el caso pequeno de referencia\n");

    Graph *g = loadGraph("tests/small10.txt");
    CHECK(g != NULL, "no se pudo leer tests/small10.txt (corre make test desde la raiz)");
    if (g == NULL) {
        return;
    }

    int parent[10];
    PrimResult r = primFibonacci(g, 0, parent, NULL);

    CHECK(r.mstEdges == 9, "el MST tiene %lld aristas y deberia tener 9", r.mstEdges);
    CHECK(fabs(r.mstWeight - SMALL10_MST_WEIGHT) < 1e-12,
          "el MST pesa %.6f y el calculado a mano pesa %.6f", r.mstWeight,
          (double)SMALL10_MST_WEIGHT);
    CHECK(r.extractMinCalls == 10, "hubo %lld extractMin y deberian ser 10", r.extractMinCalls);
    CHECK(validateTree(g, 0, parent, r.mstWeight), "parent no describe un arbol cobertor valido");

    /* En este grafo el MST es el camino 0-1-2-...-9, asi que cada vertice tiene
     * como padre al anterior. */
    int wrongParents = 0;
    for (int v = 1; v < 10; v++) {
        if (parent[v] != v - 1) {
            wrongParents++;
        }
    }
    CHECK(wrongParents == 0, "%d vertices no tienen como padre al anterior del camino",
          wrongParents);

    /* El peso del MST no depende de la raiz, aunque el arbol si pueda cambiar. */
    int differentWeights = 0;
    for (int root = 1; root < 10; root++) {
        PrimResult other = primFibonacci(g, root, NULL, NULL);
        if (fabs(other.mstWeight - r.mstWeight) > 1e-12) {
            differentWeights++;
        }
    }
    CHECK(differentWeights == 0, "%d raices dieron un MST de peso distinto", differentWeights);

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
        {512, 2048},
        {4096, 16384},
    };

    for (size_t k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        int V = cases[k].V;
        Graph *g = generateGraph(V, cases[k].E, (unsigned int)(500 + k));
        CHECK(g != NULL, "V=%d: fallo la generacion", V);
        if (g == NULL) {
            continue;
        }

        int *parent = malloc((size_t)V * sizeof *parent);
        if (parent == NULL) {
            freeGraph(g);
            CHECK(0, "sin memoria");
            continue;
        }

        PrimResult r = primFibonacci(g, 0, parent, NULL);
        CHECK(r.mstEdges == V - 1, "V=%d: el MST tiene %lld aristas y deberia tener %d",
              V, r.mstEdges, V - 1);
        CHECK(r.extractMinCalls == V, "V=%d: hubo %lld extractMin y deberian ser %d",
              V, r.extractMinCalls, V);
        CHECK(validateTree(g, 0, parent, r.mstWeight),
              "V=%d: parent no describe un arbol cobertor valido", V);

        /* Cambiar la raiz no puede cambiar el peso del MST. */
        int differentWeights = 0;
        for (int root = 1; root < V && root < 8; root++) {
            PrimResult other = primFibonacci(g, root, NULL, NULL);
            if (fabs(other.mstWeight - r.mstWeight) > 1e-9) {
                differentWeights++;
            }
        }
        CHECK(differentWeights == 0, "V=%d: %d raices dieron un MST de peso distinto",
              V, differentWeights);

        free(parent);
        freeGraph(g);
    }
}

static void testInstrumentation(void) {
    printf("Bitacora de decreaseKey\n");

    Graph *g = generateGraph(2048, 8192, 77);
    CHECK(g != NULL, "fallo la generacion");
    if (g == NULL) {
        return;
    }

    /* La corrida sin bitacora sirve de referencia: instrumentar no debe cambiar
     * el resultado del algoritmo. */
    PrimResult plain = primFibonacci(g, 0, NULL, NULL);
    CHECK(plain.decreaseKeyTime == 0.0, "sin bitacora se reporto tiempo de decreaseKey");

    DecreaseLog log;
    CHECK(decreaseLogInit(&log, g->E) == 0, "no se pudo reservar la bitacora");
    PrimResult timed = primFibonacci(g, 0, NULL, &log);

    CHECK(fabs(timed.mstWeight - plain.mstWeight) < 1e-12,
          "el MST cambio al instrumentar: %.17g contra %.17g", timed.mstWeight, plain.mstWeight);
    CHECK(timed.decreaseKeyCalls == plain.decreaseKeyCalls,
          "la cantidad de llamadas cambio al instrumentar: %lld contra %lld",
          timed.decreaseKeyCalls, plain.decreaseKeyCalls);
    CHECK(timed.decreaseKeyOps == plain.decreaseKeyOps,
          "la cantidad de cortes cambio al instrumentar: %lld contra %lld",
          timed.decreaseKeyOps, plain.decreaseKeyOps);
    CHECK(log.count == timed.decreaseKeyCalls,
          "la bitacora tiene %lld muestras y hubo %lld llamadas", log.count,
          timed.decreaseKeyCalls);
    CHECK(log.count <= log.capacity, "la bitacora se quedo corta de capacidad");

    long long sumOps = 0;
    double sumTime = 0.0;
    long long negative = 0;
    for (long long k = 0; k < log.count && k < log.capacity; k++) {
        sumOps += log.samples[k].ops;
        sumTime += log.samples[k].seconds;
        if (log.samples[k].seconds < 0.0 || log.samples[k].ops < 0) {
            negative++;
        }
    }
    CHECK(negative == 0, "%lld muestras tienen valores negativos", negative);
    CHECK(sumOps == timed.decreaseKeyOps,
          "las muestras suman %lld cortes y el total reportado es %lld", sumOps,
          timed.decreaseKeyOps);
    CHECK(fabs(sumTime - timed.decreaseKeyTime) < 1e-9,
          "las muestras suman %.9f s y el total reportado es %.9f s", sumTime,
          timed.decreaseKeyTime);

    /* La propiedad central del analisis amortizado: el total de cortes no puede
     * pasar de una constante por la cantidad de llamadas. */
    CHECK(timed.decreaseKeyOps <= 2 * timed.decreaseKeyCalls,
          "hubo %lld cortes en %lld llamadas, mas de 2 por llamada en promedio",
          timed.decreaseKeyOps, timed.decreaseKeyCalls);

    decreaseLogFree(&log);
    freeGraph(g);
}

int main(void) {
    testExtractOrder();
    testDecreaseKey();
    testFreeNonEmpty();
    testPrimSmall();
    testPrimRandom();
    testInstrumentation();

    printf("\n%d chequeos, %d fallas\n", checks, failures);
    return (failures == 0) ? 0 : 1;
}
