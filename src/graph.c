#include "graph.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* --- Generador pseudoaleatorio ------------------------------------------- */

/* Mezclador de splitmix64: dispersa los bits de z de forma que entradas
 * consecutivas produzcan salidas sin correlacion aparente. */
static uint64_t mix64(uint64_t z) {
    z += 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Se usa un generador propio en vez de rand() para que los datasets sean
 * reproducibles entre maquinas y compiladores: rand() no tiene una secuencia
 * especificada por el estandar. */
typedef struct {
    uint64_t state;
} Rng;

static uint64_t rngNext(Rng *r) {
    r->state += 0x9E3779B97F4A7C15ULL;
    return mix64(r->state);
}

/* Entero uniforme en [0, n), con n > 0. Descarta los valores del rango
 * superior que no alcanzan a repartirse en partes iguales, para que el modulo
 * no sesgue la distribucion. */
static uint64_t rngBelow(Rng *r, uint64_t n) {
    uint64_t threshold = (0ULL - n) % n;
    uint64_t x;
    do {
        x = rngNext(r);
    } while (x < threshold);
    return x % n;
}

/* Peso uniforme en (0,1]: se toman 53 bits (la mantisa de un double) y se
 * desplaza el rango en uno para excluir el 0 e incluir el 1. */
static double rngWeight(Rng *r) {
    return (double)((rngNext(r) >> 11) + 1) / 9007199254740992.0;
}

/* --- Conjunto de aristas ya insertadas ----------------------------------- */

/* Tabla de hash abierta sobre claves de 64 bits, usada solo durante la
 * generacion para detectar aristas repetidas en tiempo constante esperado.
 * Recorrer la lista de adyacencia seria O(grado), y en las configuraciones con
 * pocos vertices y muchas aristas el grado promedio pasa de 500. */
typedef struct {
    uint64_t *slots;  /* 0 marca la casilla vacia; si no, la clave mas uno */
    uint64_t mask;    /* capacidad - 1, con capacidad potencia de dos */
} EdgeSet;

/* Clave canonica de la arista no dirigida {u,v}: se ordena el par para que
 * {u,v} y {v,u} colisionen a proposito. */
static uint64_t edgeKey(int u, int v, int V) {
    if (u > v) {
        int t = u;
        u = v;
        v = t;
    }
    return (uint64_t)u * (uint64_t)V + (uint64_t)v;
}

/* Reserva la tabla con factor de carga a lo mas 0.5. Retorna 0 u -1. */
static int edgeSetInit(EdgeSet *s, long long expected) {
    uint64_t cap = 16;
    while (cap < (uint64_t)expected * 2) {
        cap <<= 1;
    }
    s->slots = calloc((size_t)cap, sizeof *s->slots);
    if (s->slots == NULL) {
        return -1;
    }
    s->mask = cap - 1;
    return 0;
}

/* Inserta la clave si no estaba. Retorna 1 si la inserto, 0 si ya estaba. */
static int edgeSetInsert(EdgeSet *s, uint64_t key) {
    uint64_t stored = key + 1;
    uint64_t i = mix64(stored) & s->mask;
    while (s->slots[i] != 0) {
        if (s->slots[i] == stored) {
            return 0;
        }
        i = (i + 1) & s->mask;
    }
    s->slots[i] = stored;
    return 1;
}

static void edgeSetFree(EdgeSet *s) {
    free(s->slots);
    s->slots = NULL;
}

/* --- API publica --------------------------------------------------------- */

Graph *createGraph(int V, long long E) {
    if (V <= 0 || E < 0) {
        return NULL;
    }

    Graph *g = malloc(sizeof *g);
    if (g == NULL) {
        return NULL;
    }

    g->V = V;
    g->E = 0;
    g->poolUsed = 0;
    g->poolCap = 2 * E;
    g->adj = calloc((size_t)V, sizeof *g->adj);
    g->pool = (E > 0) ? malloc((size_t)g->poolCap * sizeof *g->pool) : NULL;

    if (g->adj == NULL || (E > 0 && g->pool == NULL)) {
        freeGraph(g);
        return NULL;
    }
    return g;
}

int addEdge(Graph *g, int u, int v, double weight) {
    if (g == NULL || u < 0 || v < 0 || u >= g->V || v >= g->V || u == v) {
        return -1;
    }
    if (!(weight > 0.0)) {
        return -1;
    }
    if (g->poolUsed + 2 > g->poolCap) {
        return -1;
    }

    Edge *uv = &g->pool[g->poolUsed++];
    uv->to = v;
    uv->weight = weight;
    uv->next = g->adj[u];
    g->adj[u] = uv;

    Edge *vu = &g->pool[g->poolUsed++];
    vu->to = u;
    vu->weight = weight;
    vu->next = g->adj[v];
    g->adj[v] = vu;

    g->E++;
    return 0;
}

Graph *generateGraph(int V, long long E, unsigned int seed) {
    if (V <= 0) {
        return NULL;
    }

    long long maxEdges = (long long)V * (V - 1) / 2;
    if (E < (long long)V - 1 || E > maxEdges) {
        return NULL;
    }

    Graph *g = createGraph(V, E);
    if (g == NULL) {
        return NULL;
    }

    EdgeSet seen;
    if (edgeSetInit(&seen, E) != 0) {
        freeGraph(g);
        return NULL;
    }

    Rng rng;
    rng.state = mix64((uint64_t)seed);

    /* Arbol cobertor: conectar cada vertice i a uno anterior garantiza que el
     * grafo quede conexo y sin ciclos, con exactamente V-1 aristas distintas. */
    for (int i = 1; i < V; i++) {
        int j = (int)rngBelow(&rng, (uint64_t)i);
        edgeSetInsert(&seen, edgeKey(i, j, V));
        addEdge(g, i, j, rngWeight(&rng));
    }

    /* Aristas restantes al azar, descartando reflexivas y repetidas. */
    while (g->E < E) {
        int u = (int)rngBelow(&rng, (uint64_t)V);
        int v = (int)rngBelow(&rng, (uint64_t)V);
        if (u == v) {
            continue;
        }
        if (!edgeSetInsert(&seen, edgeKey(u, v, V))) {
            continue;
        }
        addEdge(g, u, v, rngWeight(&rng));
    }

    edgeSetFree(&seen);
    return g;
}

int saveGraph(const Graph *g, const char *filename) {
    if (g == NULL || filename == NULL) {
        return -1;
    }

    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        return -1;
    }

    if (fprintf(f, "%d %lld\n", g->V, g->E) < 0) {
        fclose(f);
        return -1;
    }

    for (int u = 0; u < g->V; u++) {
        for (const Edge *e = g->adj[u]; e != NULL; e = e->next) {
            /* Cada arista esta dos veces en las listas: se escribe solo la
             * copia con el extremo menor primero. */
            if (u >= e->to) {
                continue;
            }
            if (fprintf(f, "%d %d %.17g\n", u, e->to, e->weight) < 0) {
                fclose(f);
                return -1;
            }
        }
    }

    return (fclose(f) == 0) ? 0 : -1;
}

Graph *loadGraph(const char *filename) {
    if (filename == NULL) {
        return NULL;
    }

    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        return NULL;
    }

    int V;
    long long E;
    if (fscanf(f, "%d %lld", &V, &E) != 2) {
        fclose(f);
        return NULL;
    }

    Graph *g = createGraph(V, E);
    if (g == NULL) {
        fclose(f);
        return NULL;
    }

    for (long long k = 0; k < E; k++) {
        int u;
        int v;
        double w;
        if (fscanf(f, "%d %d %lf", &u, &v, &w) != 3 || addEdge(g, u, v, w) != 0) {
            freeGraph(g);
            fclose(f);
            return NULL;
        }
    }

    fclose(f);
    return g;
}

void freeGraph(Graph *g) {
    if (g == NULL) {
        return;
    }
    free(g->adj);
    free(g->pool);
    free(g);
}
