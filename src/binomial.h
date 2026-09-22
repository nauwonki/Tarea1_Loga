#ifndef BINOMIAL_H
#define BINOMIAL_H

/* ---------------------------------------------------------------------------
 * Cola binomial de pares (key, vertex) ordenados por key, con extractMin y
 * decreaseKey en O(log n) en el peor caso.
 * ------------------------------------------------------------------------- */

typedef struct BinomialNode {
    int vertex;                    /* vertice que representa el par */
    double key;                    /* costo asociado */
    int degree;                    /* cantidad de hijos */
    struct BinomialNode *parent;   /* padre, NULL si es raiz */
    struct BinomialNode *child;    /* hijo de mayor grado, NULL si es hoja */
    struct BinomialNode *sibling;  /* siguiente hermano; si el nodo es raiz,
                                    * siguiente raiz de la lista de arboles */
} BinomialNode;

typedef struct {
    BinomialNode *head;  /* lista de raices en orden creciente de grado */
    int size;            /* cantidad de nodos en la cola */
} BinomialHeap;

/* Crea una cola vacia. Retorna NULL si falla la memoria. */
BinomialHeap *createBinomialHeap(void);

/* Crea un nodo suelto con el par (key, vertex), sin padre ni hijos.
 * Retorna NULL si falla la memoria. */
BinomialNode *createBinomialNode(
    int vertex,
    double key
);

/* Inserta un nodo ya creado en la cola, uniendolo a la lista de arboles.
 * El nodo pasa a ser propiedad de la cola. */
void insertBinomialHeap(
    BinomialHeap *h,
    BinomialNode *node
);

/* Construye la cola con los n pares (keys[v], v) mediante inserciones
 * sucesivas, que cuestan O(n) en total (seccion 3.4 del enunciado).
 * Deja en nodeOf[v] el puntero al nodo que representa al vertice v, para que
 * Prim pueda llegar a el en O(1); nodeOf debe tener al menos n posiciones.
 * Retorna la cola, o NULL si falla la memoria. */
BinomialHeap *buildBinomialHeap(
    const double *keys,
    int n,
    BinomialNode **nodeOf
);

/* Extrae el nodo de menor key y lo saca de la cola.
 * Retorna el nodo, que queda desligado y pasa a ser responsabilidad del
 * llamador (debe liberarlo con free), o NULL si la cola esta vacia. */
BinomialNode *extractMinBinomialHeap(
    BinomialHeap *h
);

/* Reduce a newKey la key del nodo x subiendo su contenido por el arbol
 * mientras viole el orden de heap.
 *
 * Como lo que se intercambia es el contenido de los nodos y no los nodos
 * mismos, cada intercambio deja los punteros de nodeOf apuntando al nodo
 * equivocado, asi que esta funcion los corrige: tras cada intercambio deja
 * nodeOf[v] apuntando al nodo que ahora representa a v.
 *
 * Si swaps no es NULL, le suma la cantidad de intercambios realizados (la
 * operacion estructural que se cuenta en la seccion 6.3.2).
 * No hace nada si newKey es mayor que la key actual. */
void decreaseKeyBinomialHeap(
    BinomialHeap *h,
    BinomialNode *x,
    double newKey,
    BinomialNode **nodeOf,
    long long *swaps
);

/* Libera la cola y todos los nodos que todavia contenga. Acepta NULL. */
void deleteBinomialHeap(
    BinomialHeap *h
);

#endif
