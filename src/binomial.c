#include "binomial.h"
#include <stdio.h>
#include <stdlib.h>

/* Une dos listas ordenadas por grado */
static BinomialNode *mergeLists(BinomialNode *a, BinomialNode *b) {
    BinomialNode d;
    BinomialNode *tail = &d;

    d.sibling = NULL;
    while (a != NULL && b != NULL) {
        if (a->degree <= b->degree) {
            tail->sibling = a;
            a = a->sibling;
        } else {
            tail->sibling = b;
            b = b->sibling;
        }
        tail = tail->sibling;
    }
    if (a != NULL) {
        tail->sibling = a;
    } else {
        tail->sibling = b;
    }
    return d.sibling;
}

/* Convierte nodo y a que sea hijo del nodo z */
static void linkTrees(BinomialNode *y, BinomialNode *z) {
    y->parent = z;
    y->sibling = z->child;
    z->child = y;
    z->degree++;
}

/* Une dos heaps binomiales */
static void unionHeaps(BinomialHeap *h1, BinomialHeap *h2) {
    BinomialNode *head;
    BinomialNode *prev;
    BinomialNode *curr;
    BinomialNode *next;

    if (h1 == NULL || h2 == NULL) {
        return;
    }

    head = mergeLists(h1->head, h2->head);
    h1->head = head;
    h1->size += h2->size;

    if (head == NULL) {
        h2->head = NULL;
        h2->size = 0;
        return;
    }
    prev = NULL;
    curr = head;
    next = curr->sibling;

    while (next != NULL) {
        /* Grados distintos */
        if (curr->degree != next->degree) {
            prev = curr;
            curr = next;
            next = next->sibling;
        }
        /* Mismo grado consecutivos */
        else if ((next->sibling != NULL) && (next->sibling->degree == curr->degree)) {
            prev = curr;
            curr = next;
            next = next->sibling;
        }
        /* curr es padre */
        else if (curr->key <= next->key) {
            curr->sibling = next->sibling;
            linkTrees(next, curr);
            next = curr->sibling;
        }
        /* next es padre */
        else {
            if (prev == NULL) {
                h1->head = next;
            } else {
                prev->sibling = next;
            }
            linkTrees(curr, next);

            curr = next;
            next = curr->sibling;
        }
    }
    /* Vaciar h2 */
    h2->head = NULL;
    h2->size = 0;
}

BinomialHeap *createBinomialHeap(void) {
    BinomialHeap *h = malloc(sizeof(BinomialHeap));
    if (h == NULL) {
        return NULL;
    }

    h->head = NULL;
    h->size = 0;
    return h;
}

BinomialNode *createBinomialNode(int vertex, double key) {
    BinomialNode *node = malloc(sizeof(BinomialNode));
    if (node == NULL) {
        return NULL;
    }

    node->vertex = vertex;
    node->key = key;
    node->degree = 0;
    node->parent = NULL;
    node->child = NULL;
    node->sibling = NULL;

    return node;
}

void insertBinomialHeap(BinomialHeap *h, BinomialNode *node) {
    BinomialHeap temp;
    if (h == NULL || node == NULL) {
        return;
    }

    node->degree = 0;
    node->parent = NULL;
    node->child = NULL;
    node->sibling = NULL;

    temp.head = node;
    temp.size = 1;
    unionHeaps(h, &temp);
}

BinomialHeap *buildBinomialHeap(const double *keys, int n, BinomialNode **nodeOf) {
    BinomialHeap *h;
    int i;
    if (keys == NULL || n <= 0 || nodeOf == NULL) {
        return NULL;
    }

    h = createBinomialHeap();
    if (h == NULL) {
        return NULL;
    }

    for (i = 0; i < n; i++) {
        BinomialNode *node;
        node = createBinomialNode(i, keys[i]);
        if (node == NULL) {
            deleteBinomialHeap(h);
            return NULL;
        }
        nodeOf[i] = node;
        insertBinomialHeap(h, node);
    }
    return h;
}

BinomialNode *extractMinBinomialHeap(BinomialHeap *h) {
    BinomialNode *minNode;
    BinomialNode *prevMin;
    BinomialNode *curr;
    BinomialNode *prev;
    BinomialNode *children;
    BinomialHeap childHeap;

    if (h == NULL || h->head == NULL) {
        return NULL;
    }
    /* Buscar raiz con key min */
    minNode = h->head;
    prevMin = NULL;
    prev = NULL;
    curr = h->head;

    while (curr != NULL) {
        if (curr->key < minNode->key) {
            minNode = curr;
            prevMin = prev;
        }
        prev = curr;
        curr = curr->sibling;
    }
    /* Eliminar minNode de la lista de raíces */
    if (prevMin == NULL) {
        h->head = minNode->sibling;
    } else {
        prevMin->sibling = minNode->sibling;
    }
    /* Crear un nuevo heap con los hijos de minNode */
    children = minNode->child;
    childHeap.head = NULL;
    childHeap.size = 0;

    while (children != NULL) {
        BinomialNode *next = children->sibling;
        children->parent = NULL;
        children->sibling = childHeap.head;
        childHeap.head = children;
        children = next;
    }
    if (minNode->degree > 0) {
        childHeap.size = (1 << minNode->degree) - 1; /* 2^degree - 1 */
    }
    h->size--;
    unionHeaps(h, &childHeap);
    minNode->parent = NULL;
    minNode->child = NULL;
    minNode->sibling = NULL;
    minNode->degree = 0;
    return minNode;
}

void decreaseKeyBinomialHeap(BinomialHeap *h, BinomialNode *x, double newkey, BinomialNode **nodeOf, long long *swaps) {
    BinomialNode *y;
    (void)h;

    if (x == NULL) {
        return;
    }
    if (newkey > x->key) {
        return;
    }

    x->key = newkey;
    y = x;

    while(y->parent != NULL && y->key < y->parent->key) {
        BinomialNode *z = y->parent;
        /* Intercambiar y z */
        int tempVertex;
        double tempKey;
        tempVertex = y->vertex;
        tempKey = y->key;
        
        y->vertex = z->vertex;
        y->key = z->key;
        z->vertex = tempVertex;
        z->key = tempKey;

        /* Actualizar nodeOf */
        if (nodeOf != NULL) {
            nodeOf[y->vertex] = y;
            nodeOf[z->vertex] = z;
        }
        if (swaps != NULL) {
            (*swaps)++;
        }
        y = z;
    }
}

static void freeBinomialNode(BinomialNode *node) {
    while (node != NULL) {
        BinomialNode *next = node->sibling;
        if (node->child != NULL) {
            freeBinomialNode(node->child);
        }
        free(node);
        node = next;
    }
}

void deleteBinomialHeap(BinomialHeap *h) {
    if (h == NULL) {
        return;
    }
    freeBinomialNode(h->head);
    free(h);
}