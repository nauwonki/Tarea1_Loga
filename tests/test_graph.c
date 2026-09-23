/* Pruebas del generador de grafos y del caso pequeno de referencia.
 * Se compila y ejecuta con `make test`. */

#include "graph.h"

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

/* --- Utilidades ---------------------------------------------------------- */

/* Copia de las aristas del grafo en forma canonica, para poder ordenarlas y
 * comparar dos grafos o buscar repeticiones. */
typedef struct {
    uint64_t key;
    double weight;
} EdgeCopy;

static uint64_t canonicalKey(int u, int v, int V) {
    if (u > v) {
        int t = u;
        u = v;
        v = t;
    }
    return (uint64_t)u * (uint64_t)V + (uint64_t)v;
}

static int cmpEdgeCopy(const void *a, const void *b) {
    const EdgeCopy *x = a;
    const EdgeCopy *y = b;
    if (x->key != y->key) {
        return (x->key < y->key) ? -1 : 1;
    }
    if (x->weight != y->weight) {
        return (x->weight < y->weight) ? -1 : 1;
    }
    return 0;
}

/* Extrae las 2E entradas dirigidas del grafo, ordenadas. El llamador libera. */
static EdgeCopy *collectEdges(const Graph *g, long long *outCount) {
    long long n = 0;
    EdgeCopy *out = malloc((size_t)(2 * g->E) * sizeof *out);
    if (out == NULL) {
        *outCount = 0;
        return NULL;
    }
    for (int u = 0; u < g->V; u++) {
        for (const Edge *e = g->adj[u]; e != NULL; e = e->next) {
            if (n < 2 * g->E) {
                out[n].key = canonicalKey(u, e->to, g->V);
                out[n].weight = e->weight;
            }
            n++;
        }
    }
    qsort(out, (size_t)((n < 2 * g->E) ? n : 2 * g->E), sizeof *out, cmpEdgeCopy);
    *outCount = n;
    return out;
}

/* Recorrido en anchura desde el vertice 0. Retorna 1 si alcanza a todos. */
static int isConnected(const Graph *g) {
    char *seen = calloc((size_t)g->V, sizeof *seen);
    int *queue = malloc((size_t)g->V * sizeof *queue);
    if (seen == NULL || queue == NULL) {
        free(seen);
        free(queue);
        return -1;
    }

    int head = 0;
    int tail = 0;
    queue[tail++] = 0;
    seen[0] = 1;
    int reached = 1;

    while (head < tail) {
        int u = queue[head++];
        for (const Edge *e = g->adj[u]; e != NULL; e = e->next) {
            if (!seen[e->to]) {
                seen[e->to] = 1;
                reached++;
                queue[tail++] = e->to;
            }
        }
    }

    free(seen);
    free(queue);
    return reached == g->V;
}

/* Verifica las invariantes que el enunciado exige del dataset: cantidad exacta
 * de aristas, simplicidad, simetria de las listas, pesos en (0,1] y conexidad. */
static void checkInvariants(const Graph *g, long long expectedE, const char *label) {
    CHECK(g != NULL, "%s: el grafo es NULL", label);
    if (g == NULL) {
        return;
    }

    CHECK(g->E == expectedE, "%s: E = %lld, se esperaba %lld", label, g->E, expectedE);
    CHECK(g->poolUsed == 2 * g->E, "%s: el pool tiene %lld nodos y las aristas son %lld",
          label, g->poolUsed, g->E);

    int selfLoops = 0;
    int badWeights = 0;
    for (int u = 0; u < g->V; u++) {
        for (const Edge *e = g->adj[u]; e != NULL; e = e->next) {
            if (e->to == u) {
                selfLoops++;
            }
            if (!(e->weight > 0.0 && e->weight <= 1.0)) {
                badWeights++;
            }
        }
    }
    CHECK(selfLoops == 0, "%s: hay %d aristas reflexivas", label, selfLoops);
    CHECK(badWeights == 0, "%s: hay %d pesos fuera de (0,1]", label, badWeights);

    long long count = 0;
    EdgeCopy *edges = collectEdges(g, &count);
    CHECK(edges != NULL, "%s: sin memoria para copiar las aristas", label);
    if (edges != NULL) {
        CHECK(count == 2 * g->E, "%s: las listas tienen %lld entradas y se esperaban %lld",
              label, count, 2 * g->E);

        /* Cada arista no dirigida debe aparecer exactamente dos veces, una por
         * sentido y con el mismo peso; tres o mas apariciones serian una arista
         * repetida. */
        int asymmetric = 0;
        int duplicated = 0;
        for (long long i = 0; i + 1 < count; i += 2) {
            if (edges[i].key != edges[i + 1].key || edges[i].weight != edges[i + 1].weight) {
                asymmetric++;
            }
            if (i >= 2 && edges[i].key == edges[i - 2].key) {
                duplicated++;
            }
        }
        CHECK(asymmetric == 0, "%s: %d aristas no estan en ambas listas con el mismo peso",
              label, asymmetric);
        CHECK(duplicated == 0, "%s: %d aristas repetidas", label, duplicated);
        free(edges);
    }

    CHECK(isConnected(g) == 1, "%s: el grafo no es conexo", label);
}

/* --- Pruebas ------------------------------------------------------------- */

/* El grafo de tests/small10.txt tiene 10 vertices y 20 aristas con pesos
 * distintos. Su MST, calculado a mano con Kruskal, es el camino
 * 0-1-2-3-4-5-6-7-8-9 y pesa 0.10+0.20+0.30+0.15+0.25+0.35+0.05+0.40+0.45 =
 * 2.25. Al ser todos los pesos distintos el MST es unico, asi que sirve como
 * prueba de regresion exacta para las dos versiones de Prim. */
#define SMALL10_MST_WEIGHT 2.25

static void testSmallFixture(void) {
    printf("Caso pequeno de referencia (tests/small10.txt)\n");

    Graph *g = loadGraph("tests/small10.txt");
    CHECK(g != NULL, "no se pudo leer tests/small10.txt (corre make test desde la raiz)");
    if (g == NULL) {
        return;
    }

    CHECK(g->V == 10, "V = %d, se esperaba 10", g->V);
    checkInvariants(g, 20, "small10");

    /* Chequeo de integridad del fixture: la suma de las 9 aristas mas livianas
     * debe ser el peso del MST documentado, porque en este grafo esas 9 aristas
     * son justamente las del arbol. */
    long long count = 0;
    EdgeCopy *edges = collectEdges(g, &count);
    if (edges != NULL) {
        double *weights = malloc((size_t)g->E * sizeof *weights);
        if (weights != NULL) {
            for (long long i = 0; i < g->E; i++) {
                weights[i] = edges[2 * i].weight;
            }
            /* orden por insercion, son solo 20 elementos */
            for (long long i = 1; i < g->E; i++) {
                double w = weights[i];
                long long j = i - 1;
                while (j >= 0 && weights[j] > w) {
                    weights[j + 1] = weights[j];
                    j--;
                }
                weights[j + 1] = w;
            }
            double sum = 0.0;
            for (int i = 0; i < 9; i++) {
                sum += weights[i];
            }
            CHECK(fabs(sum - SMALL10_MST_WEIGHT) < 1e-12,
                  "las 9 aristas mas livianas suman %.6f y el MST documentado pesa %.6f",
                  sum, (double)SMALL10_MST_WEIGHT);
            free(weights);
        }
        free(edges);
    }

    printf("  MST esperado: %.2f (se verifica contra Prim en test_fibonacci)\n",
           (double)SMALL10_MST_WEIGHT);
    freeGraph(g);
}

static void testGenerator(void) {
    printf("Invariantes del generador\n");

    struct {
        int V;
        long long E;
    } cases[] = {
        {2, 1},         /* minimo posible */
        {8, 7},         /* exactamente un arbol, sin aristas extra */
        {64, 256},      /* denso en relacion a V */
        {1024, 4096},   /* grado promedio 8 */
        {4096, 16384},
    };

    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        char label[64];
        snprintf(label, sizeof label, "V=%d E=%lld", cases[i].V, cases[i].E);
        Graph *g = generateGraph(cases[i].V, cases[i].E, (unsigned int)(1000 + i));
        checkInvariants(g, cases[i].E, label);
        freeGraph(g);
    }
}

static void testReproducibility(void) {
    printf("Reproducibilidad del generador\n");

    Graph *a = generateGraph(1024, 4096, 42);
    Graph *b = generateGraph(1024, 4096, 42);
    Graph *c = generateGraph(1024, 4096, 43);
    CHECK(a != NULL && b != NULL && c != NULL, "fallo la generacion");
    if (a == NULL || b == NULL || c == NULL) {
        freeGraph(a);
        freeGraph(b);
        freeGraph(c);
        return;
    }

    long long na = 0;
    long long nb = 0;
    long long nc = 0;
    EdgeCopy *ea = collectEdges(a, &na);
    EdgeCopy *eb = collectEdges(b, &nb);
    EdgeCopy *ec = collectEdges(c, &nc);

    if (ea != NULL && eb != NULL && ec != NULL) {
        int sameSeedDiffs = 0;
        int otherSeedDiffs = 0;
        for (long long i = 0; i < na; i++) {
            if (ea[i].key != eb[i].key || ea[i].weight != eb[i].weight) {
                sameSeedDiffs++;
            }
            if (ea[i].key != ec[i].key || ea[i].weight != ec[i].weight) {
                otherSeedDiffs++;
            }
        }
        CHECK(sameSeedDiffs == 0, "la misma semilla dio %d aristas distintas", sameSeedDiffs);
        CHECK(otherSeedDiffs > 0, "dos semillas distintas dieron el mismo grafo");
    }

    free(ea);
    free(eb);
    free(ec);
    freeGraph(a);
    freeGraph(b);
    freeGraph(c);
}

static void testSaveLoad(void) {
    printf("Ida y vuelta a archivo\n");

    const char *path = "build/roundtrip.txt";
    Graph *original = generateGraph(512, 2048, 7);
    CHECK(original != NULL, "fallo la generacion");
    if (original == NULL) {
        return;
    }

    CHECK(saveGraph(original, path) == 0, "no se pudo escribir %s", path);
    Graph *reloaded = loadGraph(path);
    checkInvariants(reloaded, original->E, "releido");

    if (reloaded != NULL) {
        CHECK(reloaded->V == original->V, "V cambio de %d a %d", original->V, reloaded->V);

        long long n1 = 0;
        long long n2 = 0;
        EdgeCopy *e1 = collectEdges(original, &n1);
        EdgeCopy *e2 = collectEdges(reloaded, &n2);
        if (e1 != NULL && e2 != NULL && n1 == n2) {
            int diffs = 0;
            for (long long i = 0; i < n1; i++) {
                /* %.17g conserva el double exacto, asi que la comparacion
                 * puede ser por igualdad y no por tolerancia. */
                if (e1[i].key != e2[i].key || e1[i].weight != e2[i].weight) {
                    diffs++;
                }
            }
            CHECK(diffs == 0, "%d aristas cambiaron al releer el archivo", diffs);
        }
        free(e1);
        free(e2);
    }

    freeGraph(original);
    freeGraph(reloaded);
    remove(path);
}

static void testRejections(void) {
    printf("Parametros invalidos\n");

    CHECK(generateGraph(10, 8, 1) == NULL, "acepto E < V-1, que no puede ser conexo");
    CHECK(generateGraph(10, 46, 1) == NULL, "acepto E > V(V-1)/2, que no puede ser simple");
    CHECK(generateGraph(0, 0, 1) == NULL, "acepto V = 0");
    CHECK(loadGraph("tests/no_existe.txt") == NULL, "acepto un archivo inexistente");

    Graph *g = createGraph(4, 2);
    CHECK(g != NULL, "createGraph fallo");
    if (g != NULL) {
        CHECK(addEdge(g, 0, 0, 0.5) == -1, "acepto una arista reflexiva");
        CHECK(addEdge(g, 0, 9, 0.5) == -1, "acepto un vertice fuera de rango");
        CHECK(addEdge(g, 0, 1, 0.0) == -1, "acepto peso 0");
        CHECK(addEdge(g, 0, 1, 0.5) == 0, "rechazo una arista valida");
        CHECK(addEdge(g, 1, 2, 0.5) == 0, "rechazo una arista valida");
        CHECK(addEdge(g, 2, 3, 0.5) == -1, "acepto mas aristas que la capacidad del pool");
        freeGraph(g);
    }
}

int main(void) {
    testSmallFixture();
    testGenerator();
    testReproducibility();
    testSaveLoad();
    testRejections();

    printf("\n%d chequeos, %d fallas\n", checks, failures);
    return (failures == 0) ? 0 : 1;
}
