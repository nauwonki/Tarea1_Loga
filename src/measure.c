/* clock_gettime y CLOCK_MONOTONIC no quedan visibles al compilar en modo ISO C
 * estricto (-std=c11 -pedantic), hay que pedir explicitamente POSIX. */
#define _POSIX_C_SOURCE 199309L

#include "measure.h"

#include <stdlib.h>
#include <time.h>

double nowSeconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int decreaseLogInit(DecreaseLog *log, long long capacity) {
    if (log == NULL || capacity <= 0) {
        return -1;
    }

    log->samples = malloc((size_t)capacity * sizeof *log->samples);
    if (log->samples == NULL) {
        return -1;
    }
    log->count = 0;
    log->capacity = capacity;
    return 0;
}

void decreaseLogPush(DecreaseLog *log, double seconds, long long ops) {
    if (log == NULL) {
        return;
    }
    if (log->count < log->capacity) {
        log->samples[log->count].seconds = seconds;
        log->samples[log->count].ops = ops;
    }
    log->count++;
}

void decreaseLogFree(DecreaseLog *log) {
    if (log == NULL) {
        return;
    }
    free(log->samples);
    log->samples = NULL;
    log->count = 0;
    log->capacity = 0;
}
