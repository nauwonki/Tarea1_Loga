#ifndef GRAPH_H
#define GRAPH_H

/* ---------------------------------------------------------------------------
 * Grafo no dirigido, conexo y con pesos, representado con listas de
 * adyacencia. Cada arista {u,v} se almacena dos veces: una en la lista de u y
 * otra en la lista de v.
 *
 * Los nodos de las listas no se piden con un malloc individual cada uno, sino
 * que se entregan desde un pool contiguo de 2E elementos reservado de una vez
 * en createGraph. El caso mas grande de la tarea (e = 2^24) necesita 2^25
 * nodos de arista, y el encabezado que el allocator agrega a cada bloque
 * costaria cientos de MB extra. El pool ademas mejora la localidad al
 * recorrer las listas y hace que el consumo de memoria sea exactamente
 * predecible, que es lo que pide la estimacion de la seccion 6.2.
 * ------------------------------------------------------------------------- */

/* Arista dirigida dentro de la lista de adyacencia de un vertice. */
typedef struct Edge {
    int to;             /* vertice destino */
    double weight;      /* peso de la arista, siempre > 0 */
    struct Edge *next;  /* siguiente arista incidente al mismo vertice, NULL al final */
} Edge;

typedef struct {
    int V;              /* cantidad de vertices, numerados 0..V-1 */
    long long E;        /* cantidad de aristas no dirigidas efectivamente insertadas */
    Edge **adj;         /* adj[v]: lista de aristas incidentes a v, NULL si no tiene */
    Edge *pool;         /* pool contiguo de nodos de arista */
    long long poolUsed; /* nodos del pool ya entregados */
    long long poolCap;  /* capacidad del pool (2 * aristas reservadas) */
} Graph;

/* Reserva un grafo vacio con V vertices y espacio para E aristas no dirigidas.
 * Retorna el grafo, o NULL si los parametros son invalidos o falla la memoria.
 * El grafo retornado tiene E == 0: las aristas se agregan con addEdge. */
Graph *createGraph(
    int V,
    long long E
);

/* Inserta la arista no dirigida {u,v} con el peso dado en ambas listas de
 * adyacencia. No verifica si la arista ya existe (ese control es del llamador).
 * Retorna 0 si la inserto, o -1 si los parametros son invalidos (vertice fuera
 * de rango, arista reflexiva, peso <= 0) o si el pool ya esta lleno. */
int addEdge(
    Graph *g,
    int u,
    int v,
    double weight
);

/* Genera un grafo conexo y simple con V vertices y E aristas no dirigidas de
 * peso uniforme en (0,1], de forma reproducible a partir de seed.
 *
 * Primero inserta V-1 aristas que forman un arbol cobertor (cada vertice i se
 * conecta a uno elegido al azar en [0, i-1]), lo que garantiza conectividad, y
 * luego agrega las E-V+1 aristas restantes al azar descartando las reflexivas
 * y las repetidas.
 *
 * Retorna el grafo, o NULL si E < V-1 (no podria ser conexo), si E > V(V-1)/2
 * (no podria ser simple) o si falla la memoria. Como la segunda fase usa
 * rechazo, conviene mantener E bastante por debajo de V(V-1)/2; en las
 * configuraciones de la tarea la densidad es baja y el rechazo es despreciable. */
Graph *generateGraph(
    int V,
    long long E,
    unsigned int seed
);

/* Escribe el grafo en un archivo de texto con el formato
 *     V E
 *     u v w        (una linea por arista no dirigida, E lineas)
 * usando suficientes digitos para recuperar los pesos sin perdida.
 * Retorna 0 si tuvo exito, -1 si no pudo escribir el archivo. */
int saveGraph(
    const Graph *g,
    const char *filename
);

/* Lee un grafo escrito por saveGraph.
 * Retorna el grafo, o NULL si el archivo no existe, esta mal formado o falla
 * la memoria. No verifica conectividad ni ausencia de aristas repetidas. */
Graph *loadGraph(
    const char *filename
);

/* Libera el grafo y todas sus estructuras internas. Acepta NULL. */
void freeGraph(Graph *g);

#endif
