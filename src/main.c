#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#include "graph.h"
#include "prim.h"
#include "measure.h"
#include "report.h"

#define REPETITIONS 10
#define ROOT 0
#define MAX_CURVE_POINTS 2000
#define BASE_SEED 20260925U
#define MST_EPS 1e-9

/* Utils */
static long long pow2(int exp) {
    return 1LL << exp;
}

/* Genera semilla para cada serie */
static unsigned int seedExperiment(char serie, int i, int j, int repetition) {
    uint32_t x = BASE_SEED;

    x ^= (uint32_t)(unsigned char)serie * 0x9E3779B9U;
    x ^= (uint32_t)i * 0x85EBCA6BU;
    x ^= (uint32_t)j * 0xC2B2AE35U;
    x ^= (uint32_t)repetition * 0x27D4EB2FU;

    x ^= (x >> 16);
    x *= 0x7FEB352DU;
    x ^= (x >> 15);
    x *= 0x846CA68BU;
    x ^= (x >> 16);

    return x;
}

/* Comprueba que los dos Prims obtuvieron un MST con el mismo peso 
 * y V - 1 aristas 
 */
static int sameMST(const PrimResult *a, const PrimResult *b, int V) {
    if (a->mstEdges != V - 1) {
        return 0;
    }
    if (b->mstEdges != V - 1) {
        return 0;
    }
    return fabs(a->mstWeight - b->mstWeight) <= MST_EPS;
}

/* runs.csv vacío al comenzar una ejecución */
static int resetRunsCSV(void) {
    FILE *f = fopen("results/runs.csv", "w");
    if (f == NULL) {
        return -1;
    }
    return (fclose(f) == 0) ? 0 : -1;
}

/* Configuración */
static int runConfig(char serie, int i, int j, int inst) {
    int V = (int)pow2(i);
    long long E = pow2(j);

    printf("Serie %c: V^2%d=%d, E^2%d=%lld\n", serie, i, V, j, E);
    for (int r = 0; r < REPETITIONS; r++) {
        unsigned int seed = seedExperiment(serie, i, j, r);
        printf("[%c] repeticion %d/%d\n", serie, r + 1, REPETITIONS);
        printf("Generando grafo V=%d, E=%lld, seed=%u ... ", V, E, seed);
        /* Tiempo no forma parte de totalTime */
        Graph *g = generateGraph(V, E, seed);
        if (g == NULL) {
            fprintf(stderr, "Error: no se pudo generar el grafo.\n");
            return -1;
        }

        /* ----- Binomial ----- */
        PrimResult binomialResult;
        if (!inst) {
            /* Series A/B */
            binomialResult = primBinomial(g, ROOT, NULL, NULL);
        } else {
            /* Series C/D */
            DecreaseLog log;
            if (decreaseLogInit(&log, 2*E+1) != 0) {
                fprintf(stderr, "Error: no se pudo inicializar el log de decreaseKey.\n");
                freeGraph(g);
                return -1;
            }
            binomialResult = primBinomial(g, ROOT, NULL, &log);
            RunId id = {
                .serie = NULL,
                .queue = "binomial",
                .i = i,
                .j = j,
                .repetition = r
            };
            char serieString[2] = {serie, '\0'};
            id.serie = serieString;

            if (reportDecreaseCurve("results", &id, &log, MAX_CURVE_POINTS) < 0) {
                fprintf(stderr, "Error: no se pudo volcar la curva de decreaseKey a CSV.\n");
                decreaseLogFree(&log);
                freeGraph(g);
                return -1;
            }
            decreaseLogFree(&log);
        }

        /* ----- Fibonacci ----- */
        PrimResult fibonacciResult;
        if (!inst) {
            fibonacciResult = primFibonacci(g, ROOT, NULL, NULL);
        } else {
            DecreaseLog log;
            if (decreaseLogInit(&log, 2*E+1) != 0) {
                fprintf(stderr, "Error: no se pudo inicializar el log de decreaseKey.\n");
                freeGraph(g);
                return -1;
            }
            fibonacciResult = primFibonacci(g, ROOT, NULL, &log);
            RunId id = {
                .serie = NULL,
                .queue = "fibonacci",
                .i = i,
                .j = j,
                .repetition = r
            };
            char serieString[2] = {serie, '\0'};
            id.serie = serieString;
            if (reportDecreaseCurve("results", &id, &log, MAX_CURVE_POINTS) < 0) {
                fprintf(stderr, "Error: no se pudo volcar la curva de decreaseKey a CSV.\n");
                decreaseLogFree(&log);
                freeGraph(g);
                return -1;
            }
            decreaseLogFree(&log);
        }

        /* Verificación */
        if (!sameMST(&binomialResult, &fibonacciResult, V)) {
            fprintf(
                stderr,
                "Error: los resultados de Prim con colas binomial y fibonacci no coinciden.\n"
                "Binomial: peso=%.17g, aristas=%lld\n"
                "Fibonacci: peso=%.17g, aristas=%lld\n\n",
                binomialResult.mstWeight, binomialResult.mstEdges,
                fibonacciResult.mstWeight, fibonacciResult.mstEdges
            );
            freeGraph(g);
            return -1;
        }

        /* Guardar resultados */
        char serieString[2] = {serie, '\0'};
        RunId binId = {
            .serie = serieString,
            .queue = "binomial",
            .i = i,
            .j = j,
            .repetition = r
        };
        RunId fibId = {
            .serie = serieString,
            .queue = "fibonacci",
            .i = i,
            .j = j,
            .repetition = r
        };
        if (reportRunAppend("results/runs.csv", &binId, &binomialResult) != 0) {
            fprintf(stderr, "Error: no se pudo volcar el resultado de Prim con cola binomial a CSV.\n");
            freeGraph(g);
            return -1;
        }
        if (reportRunAppend("results/runs.csv", &fibId, &fibonacciResult) != 0) {
            fprintf(stderr, "Error: no se pudo volcar el resultado de Prim con cola fibonacci a CSV.\n");
            freeGraph(g);
            return -1;
        }

        /* Resumen */
        printf("MST peso=%.12f\n", binomialResult.mstWeight);
        printf("Binomial: %.6f s | extractMin=%lld | decreaseKey=%lld\n", binomialResult.totalTime, binomialResult.extractMinCalls, binomialResult.decreaseKeyCalls);
        printf("Fibonacci: %.6f s | extractMin=%lld | decreaseKey=%lld\n", fibonacciResult.totalTime, fibonacciResult.extractMinCalls, fibonacciResult.decreaseKeyCalls);
        if (inst) {
            printf("Binomial: decreaseKey ops=%lld time=%.6g s\n", binomialResult.decreaseKeyOps, binomialResult.decreaseKeyTime);
            printf("Fibonacci: decreaseKey ops=%lld time=%.6g s\n", fibonacciResult.decreaseKeyOps, fibonacciResult.decreaseKeyTime);
        }
        freeGraph(g);
    }
    return 0;
}

/* Serie A */
static int runSeriesA(void) {
    printf("Serie A");
    const int i = 20;
    for (int j = 20; j <= 24; j++) {
        if (runConfig('A', i, j, 0) != 0) {
            return -1;
        }
    }
    return 0;
}

/* Serie B */
static int runSeriesB(void) {
    printf("Serie B");
    const int j = 24;
    for (int i = 18; i <= 22; i++) {
        if (runConfig('B', i, j, 0) != 0) {
            return -1;
        }
    }
    return 0;
}

/* Serie C */
static int runSeriesC(void) {
    printf("Serie C");
    const int i = 18;
    for (int j = 18; j <= 22; j++) {
        if (runConfig('C', i, j, 1) != 0) {
            return -1;
        }
    }
    return 0;
}

/* Serie D */
static int runSeriesD(void) {
    printf("Serie D");
    const int j = 22;
    for (int i = 14; i <= 18; i++) {
        if (runConfig('D', i, j, 1) != 0) {
            return -1;
        }
    }
    return 0;
}

/* Punto de entrada de la bateria de experimentos.
 *
 * Pendiente: series A y B (tiempo total, seccion 6.3.1), series C y D (costo
 * amortizado de decreaseKey, seccion 6.3.2), 10 repeticiones por
 * configuracion con un grafo distinto en cada una, verificacion de que ambas
 * colas producen un MST del mismo peso, y volcado de resultados a CSV. */
int main(void) {
    printf("Experimentos Prim");
    /* Crear directorio results/ */
    if (reportEnsureDir("results") != 0) {
        fprintf(stderr, "Error: no se pudo crear directorio");
        return -1;
    }

    if (resetRunsCSV() != 0) {
        fprintf(stderr, "Error: no se pudo crear runs.csv");
        return -1;
    }

    /* 6.3.1 */
    if (runSeriesA() != 0) {
        return 1;
    }
    if (runSeriesB() != 0) {
        return 1;
    }

    /* 6.3.2 */
    if (runSeriesC() != 0) {
        return 1;
    }
    if (runSeriesD() != 0) {
        return 1;
    }

    printf("Experimentos terminados\n"
    "Resultados: results/runs.csv\n"
    "Curvas: results/decrease_*.csv\n");

    return 0;
}
