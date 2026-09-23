# Tarea 1 — Algoritmo de Prim y análisis amortizado

Implementación del algoritmo de Prim
sobre dos colas de prioridad distintas (binomial y de Fibonacci) y comparación
empírica de su costo.

## Requisitos

- `gcc` con soporte de C11 (probado con gcc 13.3.0)
- `make`
- Un sistema POSIX, porque la medición de tiempos usa `clock_gettime`

## Compilar

```sh
make
```

Deja dos binarios en `build/`:

- `build/tarea1`: batería de experimentos
- `build/test_graph`: pruebas del generador de grafos

Para compilar con otras opciones se pueden pasar por línea de comandos, por
ejemplo `make CFLAGS="-std=c11 -O3 -march=native"`. Las opciones por defecto
son `-std=c11 -O2 -Wall -Wextra -pedantic`.

```sh
make clean    # borra build/
```

## Correr las pruebas

```sh
make test
```

Hay que ejecutarlo desde la raíz del repositorio, porque los tests leen el
grafo de referencia `tests/small10.txt`. Ese archivo tiene 10 vértices y 20
aristas de pesos distintos, y su MST (calculado a mano) es el camino
`0-1-2-3-4-5-6-7-8-9`, de peso **2.25**. Como todos los pesos son distintos el
MST es único, así que sirve de prueba de regresión exacta para las dos
versiones de Prim.

## Correr los experimentos

```sh
./build/tarea1
```

> Pendiente: todavía no está implementada la batería de experimentos.

Deja los resultados en `results/`: un `runs.csv` con una fila por corrida y un
`decrease_<cola>_<serie>_i<i>_j<j>_r<rep>.csv` por corrida instrumentada con la
curva acumulada de `decreaseKey`.

## Generar los gráficos y la tabla

Requiere Python 3 con `numpy` y `matplotlib`.

```sh
make plots          # equivale a python3 scripts/plot.py
```

Lee `results/` y deja en `figures/` los doce gráficos pedidos (cuatro de tiempo
total y ocho de costo amortizado), la tabla de tiempos promedio en `.csv` y
`.md`, y la verificación de que ambas colas producen un MST del mismo peso.
Todos los promedios, los ajustes de constantes de las cotas teóricas y las
interpolaciones se hacen acá y no en C, para que el código que se está midiendo
no cargue con el post-procesamiento.

## Estructura

| Archivo | Contenido |
| --- | --- |
| `src/graph.h`, `src/graph.c` | Grafo con listas de adyacencia y generador de grafos aleatorios conexos |
| `src/measure.h`, `src/measure.c` | Reloj monótono y bitácora de las llamadas a `decreaseKey` |
| `src/report.h`, `src/report.c` | Volcado de las mediciones a CSV |
| `src/binomial.h`, `src/binomial.c` | Cola binomial |
| `src/fibonacci.h`, `src/fibonacci.c` | Cola de Fibonacci |
| `src/prim.h` | Contrato común de las dos versiones de Prim |
| `src/prim_binomial.c`, `src/prim_fibonacci.c` | Prim sobre cada cola, en archivos separados para poder trabajarlas en paralelo |
| `src/main.c` | Batería de experimentos |
| `scripts/plot.py` | Gráficos, tabla y verificación del MST |
| `tests/` | Pruebas y grafo de referencia |

## Formato de los datasets

`saveGraph` y `loadGraph` usan un archivo de texto con una línea de cabecera y
una línea por arista no dirigida:

```
V E
u v w
...
```

Los pesos se escriben con `%.17g`, que permite recuperar el `double` exacto, de
modo que guardar y volver a leer un grafo no cambia ningún peso. Los datasets
generados no se versionan (ver `.gitignore`).

## Cómo instrumentar una corrida

`src/main.c` mide cada configuración así. Con `log == NULL` no se cronometra
cada llamada a `decreaseKey`, y ese es el modo que hay que usar para el tiempo
total: cronometrar llamada por llamada agrega dos lecturas del reloj a cada una,
que es un costo comparable al de la propia operación cuando es $O(1)$.

```c
/* Tiempo total (series A y B): sin bitácora. */
RunId id = {"A", "fibonacci", i, j, rep};
PrimResult r = primFibonacci(g, 0, NULL, NULL);
reportRunAppend("results/runs.csv", &id, &r);

/* Costo amortizado (series C y D): con bitácora. */
DecreaseLog log;
decreaseLogInit(&log, g->E);            /* cota: nunca hay más llamadas que aristas */
PrimResult r = primFibonacci(g, 0, NULL, &log);
reportRunAppend("results/runs.csv", &id, &r);
reportDecreaseCurve("results", &id, &log, 200);
decreaseLogFree(&log);
```

## Estado

- [x] Generador de grafos aleatorios conexos
- [x] Infraestructura de medición y volcado a CSV
- [x] Cola de Fibonacci con cortes en cascada
- [x] Prim sobre la cola de Fibonacci
- [x] Gráficos, tabla y verificación del MST
- [ ] Cola binomial
- [ ] Prim sobre la cola binomial
- [ ] Batería de experimentos en `src/main.c`
