#ifndef FIBONACCI_H
#define FIBONACCI_H

/* ---------------------------------------------------------------------------
 * Cola de Fibonacci de pares (key, vertex) ordenados por key, con extractMin
 * en O(log n) amortizado y decreaseKey en O(1) amortizado.
 *
 * La lista de arboles y las listas de hijos son circulares doblemente
 * enlazadas, lo que permite cortar un nodo de su padre en O(1).
 * ------------------------------------------------------------------------- */

typedef struct FibonacciNode {
    int vertex;                    /* vertice que representa el par */
    double key;                    /* costo asociado */
    int degree;                    /* cantidad de hijos */
    int marked;                    /* 1 si perdio un hijo desde que fue raiz por
                                    * ultima vez (el flag de los cortes en cascada) */
    struct FibonacciNode *parent;  /* padre, NULL si es raiz */
    struct FibonacciNode *child;   /* algun hijo, NULL si es hoja */
    struct FibonacciNode *left;    /* hermano anterior en la lista circular */
    struct FibonacciNode *right;   /* hermano siguiente en la lista circular */
} FibonacciNode;

typedef struct {
    FibonacciNode *min;  /* raiz de menor key, NULL si la cola esta vacia */
    int n;               /* cantidad de nodos en la cola */
} FibonacciHeap;

/* Crea una cola vacia. Retorna NULL si falla la memoria. */
FibonacciHeap *createFibonacciHeap(void);

/* Crea un nodo suelto con el par (key, vertex), sin marcar y sin hijos.
 * Retorna NULL si falla la memoria. */
FibonacciNode *createFibonacciNode(
    int vertex,
    double key
);

/* Inserta un nodo ya creado como raiz y actualiza el minimo si corresponde.
 * Cuesta O(1); el nodo pasa a ser propiedad de la cola. */
void insertFibonacciHeap(
    FibonacciHeap *h,
    FibonacciNode *x
);

/* Construye la cola con los n pares (keys[v], v) mediante inserciones
 * sucesivas, que cuestan O(n) en total (seccion 3.4 del enunciado).
 * Deja en nodeOf[v] el puntero al nodo que representa al vertice v; a
 * diferencia de la cola binomial, estos punteros nunca cambian, porque
 * decreaseKey mueve nodos completos y no su contenido.
 * Retorna la cola, o NULL si falla la memoria. */
FibonacciHeap *buildFibonacciHeap(
    const double *keys,
    int n,
    FibonacciNode **nodeOf
);

/* Extrae el nodo de menor key, promueve a sus hijos a raices y consolida la
 * lista de arboles para que quede a lo mas un arbol por grado.
 * Retorna el nodo, que queda desligado y pasa a ser responsabilidad del
 * llamador (debe liberarlo con free), o NULL si la cola esta vacia. */
FibonacciNode *extractMinFibonacciHeap(
    FibonacciHeap *h
);

/* Reduce a newKey la key del nodo x. Si con eso x viola el orden de heap, lo
 * corta de su padre y lo lleva a la lista de arboles, y aplica cortes en
 * cascada hacia arriba mientras encuentre padres ya marcados.
 *
 * Si cuts no es NULL, le suma la cantidad de cortes realizados (la operacion
 * estructural que se cuenta en la seccion 6.3.2).
 * No hace nada si newKey es mayor que la key actual. */
void decreaseKeyFibonacciHeap(
    FibonacciHeap *h,
    FibonacciNode *x,
    double newKey,
    long long *cuts
);

/* Libera la cola y todos los nodos que todavia contenga. Acepta NULL. */
void deleteFibonacciHeap(
    FibonacciHeap *h
);

#endif
