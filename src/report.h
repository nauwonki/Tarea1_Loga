#ifndef REPORT_H
#define REPORT_H

#include "prim.h"

/* ---------------------------------------------------------------------------
 * Volcado de las mediciones a CSV. Se escribe una fila por corrida y una curva
 * por corrida instrumentada; los promedios, los ajustes de constantes y los
 * graficos los calcula despues scripts/plot.py, para que el codigo medido no
 * cargue con el post-procesamiento.
 * ------------------------------------------------------------------------- */

/* Identifica una corrida dentro de la bateria de experimentos. */
typedef struct {
    const char *serie;  /* "A", "B", "C" o "D" */
    const char *queue;  /* "binomial" o "fibonacci" */
    int i;              /* v = 2^i */
    int j;              /* e = 2^j */
    int repetition;     /* numero de repeticion, desde 1 */
} RunId;

/* Crea el directorio si hace falta. Retorna 0 si existe o lo creo, -1 si no. */
int reportEnsureDir(
    const char *dir
);

/* Agrega al CSV una fila con los resultados de la corrida, escribiendo la
 * cabecera si el archivo todavia no existia.
 * Retorna 0 si tuvo exito, -1 si no pudo escribir. */
int reportRunAppend(
    const char *path,
    const RunId *id,
    const PrimResult *result
);

/* Escribe la curva acumulada de decreaseKey de una corrida en el archivo
 * <dir>/decrease_<queue>_<serie>_i<i>_j<j>_r<rep>.csv, con las columnas
 * calls, cum_seconds y cum_ops.
 *
 * Si la bitacora tiene mas de maxPoints muestras se submuestrea de forma
 * uniforme, conservando siempre la ultima: la curva es acumulada y monotona,
 * asi que unos cientos de puntos la describen igual que millones y el archivo
 * queda manejable. Con maxPoints <= 0 se escriben todas las muestras.
 *
 * Retorna 0 si tuvo exito, -1 si no pudo escribir, y 1 si la bitacora se quedo
 * corta de capacidad y hubo muestras descartadas (la curva se escribe igual,
 * pero truncada). */
int reportDecreaseCurve(
    const char *dir,
    const RunId *id,
    const DecreaseLog *log,
    long long maxPoints
);

#endif
