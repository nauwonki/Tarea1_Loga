/* mkdir vive en POSIX, no en ISO C. */
#define _POSIX_C_SOURCE 200809L

#include "report.h"

#include <stdio.h>
#include <sys/stat.h>

int reportEnsureDir(const char *dir) {
    if (dir == NULL) {
        return -1;
    }
    if (mkdir(dir, 0755) == 0) {
        return 0;
    }

    /* Puede haber fallado porque ya existe, que es un exito para el llamador. */
    struct stat st;
    if (stat(dir, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 0;
    }
    return -1;
}

int reportRunAppend(const char *path, const RunId *id, const PrimResult *result) {
    if (path == NULL || id == NULL || result == NULL) {
        return -1;
    }

    /* Se escribe la cabecera solo la primera vez, para que main pueda llamar a
     * esta funcion en cada corrida sin llevar la cuenta. */
    int needsHeader = 1;
    FILE *probe = fopen(path, "r");
    if (probe != NULL) {
        needsHeader = (fgetc(probe) == EOF);
        fclose(probe);
    }

    FILE *f = fopen(path, "a");
    if (f == NULL) {
        return -1;
    }

    if (needsHeader) {
        fprintf(f, "serie,queue,i,j,rep,v,e,mst_weight,mst_edges,total_time,"
                   "extract_min_calls,decrease_key_calls,decrease_key_ops,decrease_key_time\n");
    }

    fprintf(f, "%s,%s,%d,%d,%d,%lld,%lld,%.17g,%lld,%.9g,%lld,%lld,%lld,%.9g\n",
            id->serie, id->queue, id->i, id->j, id->repetition,
            1LL << id->i, 1LL << id->j,
            result->mstWeight, result->mstEdges, result->totalTime,
            result->extractMinCalls, result->decreaseKeyCalls,
            result->decreaseKeyOps, result->decreaseKeyTime);

    return (fclose(f) == 0) ? 0 : -1;
}

int reportDecreaseCurve(const char *dir, const RunId *id, const DecreaseLog *log,
                        long long maxPoints) {
    if (dir == NULL || id == NULL || log == NULL || log->samples == NULL) {
        return -1;
    }

    char path[512];
    if (snprintf(path, sizeof path, "%s/decrease_%s_%s_i%d_j%d_r%d.csv",
                 dir, id->queue, id->serie, id->i, id->j, id->repetition) >= (int)sizeof path) {
        return -1;
    }

    /* Si la bitacora se lleno, count siguio creciendo pero las muestras de mas
     * se descartaron: solo se pueden escribir las que si quedaron guardadas. */
    long long available = (log->count < log->capacity) ? log->count : log->capacity;
    int truncated = (log->count > log->capacity);

    long long stride = 1;
    if (maxPoints > 0 && available > maxPoints) {
        stride = available / maxPoints;
    }

    FILE *f = fopen(path, "w");
    if (f == NULL) {
        return -1;
    }
    fprintf(f, "calls,cum_seconds,cum_ops\n");

    double cumSeconds = 0.0;
    long long cumOps = 0;
    for (long long k = 0; k < available; k++) {
        cumSeconds += log->samples[k].seconds;
        cumOps += log->samples[k].ops;
        /* Se emite un punto cada stride muestras, mas siempre el ultimo, para
         * que la curva llegue hasta el total de llamadas. */
        if ((k + 1) % stride == 0 || k + 1 == available) {
            fprintf(f, "%lld,%.9g,%lld\n", k + 1, cumSeconds, cumOps);
        }
    }

    if (fclose(f) != 0) {
        return -1;
    }
    return truncated ? 1 : 0;
}
