#ifndef MEASURE_H
#define MEASURE_H

/* ---------------------------------------------------------------------------
 * Infraestructura de medicion compartida por las dos versiones de Prim, para
 * que ambas usen exactamente el mismo reloj y el mismo formato de bitacora.
 * ------------------------------------------------------------------------- */

/* Instante actual en segundos, leido de un reloj monotono de alta resolucion
 * (CLOCK_MONOTONIC). Solo tiene sentido restar dos lecturas entre si. */
double nowSeconds(void);

/* Medicion de una unica llamada a decreaseKey. */
typedef struct {
    double seconds;  /* duracion de la llamada */
    long long ops;   /* operaciones estructurales: intercambios en la cola
                      * binomial, cortes en cascada en la de Fibonacci */
} DecreaseSample;

/* Bitacora con las llamadas a decreaseKey en orden de ocurrencia.
 *
 * Se llena durante la corrida y se procesa despues de que el algoritmo
 * termina, como recomienda la seccion 6.3.2, para no contaminar la medicion
 * con el costo de calcular promedios o acumulados.
 *
 * Ojo: cronometrar cada llamada por separado le agrega a cada una el costo de
 * dos lecturas del reloj, comparable al costo de la propia operacion cuando es
 * O(1). Por eso el experimento de tiempo total (6.3.1) debe correrse con la
 * bitacora desactivada (log == NULL) y solo el de costo amortizado (6.3.2) con
 * ella activa. */
typedef struct {
    DecreaseSample *samples;  /* arreglo de 'count' muestras, o NULL */
    long long count;          /* muestras registradas */
    long long capacity;       /* muestras que caben sin descartar */
} DecreaseLog;

/* Reserva espacio para 'capacity' muestras y deja la bitacora vacia.
 * Retorna 0 si tuvo exito, -1 si falla la memoria o capacity <= 0. */
int decreaseLogInit(
    DecreaseLog *log,
    long long capacity
);

/* Agrega una muestra al final de la bitacora. Es un no-op si log es NULL, de
 * modo que Prim puede llamarla sin preguntar si la medicion esta activa.
 * Si la bitacora ya esta llena, incrementa count pero descarta la muestra, lo
 * que permite detectar despues que la capacidad quedo corta. */
void decreaseLogPush(
    DecreaseLog *log,
    double seconds,
    long long ops
);

/* Libera el arreglo de muestras y deja la bitacora en cero. Acepta NULL. */
void decreaseLogFree(DecreaseLog *log);

#endif
