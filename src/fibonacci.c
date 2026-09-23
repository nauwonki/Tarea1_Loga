#include "fibonacci.h"

#include <stdlib.h>

/* Cota para el grado maximo de una raiz despues de consolidar: en una cola de
 * Fibonacci con n nodos el grado es a lo mas log_phi(n), asi que 64 casillas
 * alcanzan hasta n = phi^64 ~ 2.3e13, muy por encima de los 2^22 nodos del
 * experimento mas grande. */
#define FIB_MAX_DEGREE 64

/* --- Listas circulares doblemente enlazadas ------------------------------ */

/* Inserta x inmediatamente a la derecha de node. Asume que x esta aislado. */
static void listInsert(FibonacciNode *node, FibonacciNode *x) {
    x->left = node;
    x->right = node->right;
    node->right->left = x;
    node->right = x;
}

/* Saca x de su lista y lo deja aislado (apuntandose a si mismo). */
static void listRemove(FibonacciNode *x) {
    x->left->right = x->right;
    x->right->left = x->left;
    x->left = x;
    x->right = x;
}

/* --- Creacion e insercion ------------------------------------------------ */

FibonacciHeap *createFibonacciHeap(void) {
    FibonacciHeap *h = malloc(sizeof *h);
    if (h == NULL) {
        return NULL;
    }
    h->min = NULL;
    h->n = 0;
    return h;
}

FibonacciNode *createFibonacciNode(int vertex, double key) {
    FibonacciNode *x = malloc(sizeof *x);
    if (x == NULL) {
        return NULL;
    }
    x->vertex = vertex;
    x->key = key;
    x->degree = 0;
    x->marked = 0;
    x->parent = NULL;
    x->child = NULL;
    x->left = x;
    x->right = x;
    return x;
}

void insertFibonacciHeap(FibonacciHeap *h, FibonacciNode *x) {
    if (h == NULL || x == NULL) {
        return;
    }

    x->parent = NULL;
    x->child = NULL;
    x->degree = 0;
    x->marked = 0;
    x->left = x;
    x->right = x;

    if (h->min == NULL) {
        h->min = x;
    } else {
        listInsert(h->min, x);
        if (x->key < h->min->key) {
            h->min = x;
        }
    }
    h->n++;
}

FibonacciHeap *buildFibonacciHeap(const double *keys, int n, FibonacciNode **nodeOf) {
    if (keys == NULL || n < 0 || nodeOf == NULL) {
        return NULL;
    }

    FibonacciHeap *h = createFibonacciHeap();
    if (h == NULL) {
        return NULL;
    }

    /* Cada insercion cuesta O(1) porque solo empalma un nodo en la lista de
     * arboles, asi que construir la cola con las n claves cuesta O(n). */
    for (int v = 0; v < n; v++) {
        FibonacciNode *x = createFibonacciNode(v, keys[v]);
        if (x == NULL) {
            deleteFibonacciHeap(h);
            return NULL;
        }
        insertFibonacciHeap(h, x);
        nodeOf[v] = x;
    }
    return h;
}

/* --- extractMin y consolidacion ------------------------------------------ */

/* Convierte a y en hijo de x. Ambos deben estar aislados de la lista de
 * raices. Al dejar de ser raiz, y pierde la marca. */
static void fibLink(FibonacciNode *y, FibonacciNode *x) {
    y->parent = x;
    y->marked = 0;
    if (x->child == NULL) {
        y->left = y;
        y->right = y;
        x->child = y;
    } else {
        listInsert(x->child, y);
    }
    x->degree++;
}

/* Une las raices de igual grado hasta que quede a lo mas una por grado. */
static void consolidate(FibonacciHeap *h) {
    FibonacciNode *table[FIB_MAX_DEGREE];
    for (int d = 0; d < FIB_MAX_DEGREE; d++) {
        table[d] = NULL;
    }

    /* Se rompe el circulo de la lista de raices para poder recorrerla como una
     * lista lineal: enlazar raices entre si modifica la lista mientras se la
     * recorre, y con el circulo intacto habria que reservar un arreglo
     * auxiliar en cada extractMin. */
    FibonacciNode *pending = h->min;
    pending->left->right = NULL;

    while (pending != NULL) {
        FibonacciNode *x = pending;
        pending = pending->right;
        x->left = x;
        x->right = x;

        int d = x->degree;
        while (d < FIB_MAX_DEGREE && table[d] != NULL) {
            FibonacciNode *y = table[d];
            if (y->key < x->key) {
                FibonacciNode *t = x;
                x = y;
                y = t;
            }
            fibLink(y, x);
            table[d] = NULL;
            d++;
        }
        table[d] = x;
    }

    /* Se rearma la lista de raices desde la tabla y se recalcula el minimo. */
    h->min = NULL;
    for (int d = 0; d < FIB_MAX_DEGREE; d++) {
        FibonacciNode *r = table[d];
        if (r == NULL) {
            continue;
        }
        r->parent = NULL;
        if (h->min == NULL) {
            h->min = r;
        } else {
            listInsert(h->min, r);
            if (r->key < h->min->key) {
                h->min = r;
            }
        }
    }
}

FibonacciNode *extractMinFibonacciHeap(FibonacciHeap *h) {
    if (h == NULL || h->min == NULL) {
        return NULL;
    }

    FibonacciNode *z = h->min;

    /* Los hijos del minimo pasan a ser raices. Se recorre la lista de hijos
     * guardando el siguiente antes de reenlazar cada uno, porque empalmarlo en
     * la lista de raices le rompe los punteros a la lista de hijos. */
    FibonacciNode *child = z->child;
    if (child != NULL) {
        z->child = NULL;
        FibonacciNode *c = child;
        do {
            FibonacciNode *next = c->right;
            c->parent = NULL;
            c->marked = 0; /* una raiz nunca queda marcada */
            listInsert(z, c);
            c = next;
        } while (c != child);
    }

    if (z == z->right) {
        h->min = NULL;
    } else {
        h->min = z->right;
        listRemove(z);
        consolidate(h);
    }

    h->n--;
    z->left = z;
    z->right = z;
    z->parent = NULL;
    z->child = NULL;
    z->degree = 0;
    return z;
}

/* --- decreaseKey, cortes y cortes en cascada ----------------------------- */

/* Saca x de la lista de hijos de y y lo agrega a la lista de arboles. */
static void fibCut(FibonacciHeap *h, FibonacciNode *x, FibonacciNode *y, long long *cuts) {
    /* Si x era el hijo por el que y apunta a su lista, hay que mover ese
     * puntero antes de desenlazarlo. */
    if (y->child == x) {
        y->child = (x->right != x) ? x->right : NULL;
    }
    listRemove(x);
    y->degree--;

    listInsert(h->min, x);
    x->parent = NULL;
    x->marked = 0;

    if (cuts != NULL) {
        (*cuts)++;
    }
}

/* Propaga los cortes hacia la raiz: un nodo que pierde su segundo hijo tambien
 * se corta. Se implementa iterativo y no recursivo para no depender de la
 * profundidad de la pila. */
static void fibCascadingCut(FibonacciHeap *h, FibonacciNode *y, long long *cuts) {
    FibonacciNode *z = y->parent;
    while (z != NULL) {
        if (!y->marked) {
            y->marked = 1;
            return;
        }
        fibCut(h, y, z, cuts);
        y = z;
        z = y->parent;
    }
}

void decreaseKeyFibonacciHeap(FibonacciHeap *h, FibonacciNode *x, double newKey, long long *cuts) {
    if (h == NULL || x == NULL || h->min == NULL || newKey > x->key) {
        return;
    }

    x->key = newKey;

    /* Si la nueva clave viola el orden de heap, se corta x de su padre en O(1)
     * en vez de subirlo hasta su posicion, que costaria O(log n). */
    FibonacciNode *y = x->parent;
    if (y != NULL && x->key < y->key) {
        fibCut(h, x, y, cuts);
        fibCascadingCut(h, y, cuts);
    }

    if (x->key < h->min->key) {
        h->min = x;
    }
}

/* --- Liberacion ---------------------------------------------------------- */

/* Libera una lista circular de nodos y todos sus descendientes. Se usa el
 * puntero right como enlace de una pila explicita para no recursionar. */
static void freeNodes(FibonacciNode *ring) {
    if (ring == NULL) {
        return;
    }

    FibonacciNode *stack = ring;
    ring->left->right = NULL;

    while (stack != NULL) {
        FibonacciNode *x = stack;
        stack = x->right;

        if (x->child != NULL) {
            FibonacciNode *c = x->child;
            c->left->right = NULL;
            FibonacciNode *tail = c;
            while (tail->right != NULL) {
                tail = tail->right;
            }
            tail->right = stack;
            stack = c;
        }
        free(x);
    }
}

void deleteFibonacciHeap(FibonacciHeap *h) {
    if (h == NULL) {
        return;
    }
    freeNodes(h->min);
    free(h);
}
