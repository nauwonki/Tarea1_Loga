# Tarea 1 — Algoritmo de Prim y análisis amortizado

CC4102 Diseño y Análisis de Algoritmos. Implementación del algoritmo de Prim
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

Hay que ejecutarlo desde la raíz del repositorio, porque el test lee el grafo
de referencia `tests/small10.txt`. Ese archivo tiene 10 vértices y 20 aristas
de pesos distintos, y su MST (calculado a mano) es el camino
`0-1-2-3-4-5-6-7-8-9`, de peso **2.25**.

## Correr los experimentos

```sh
./build/tarea1
```

> Pendiente: todavía no está implementada la batería de experimentos.

## Estructura

| Archivo | Contenido |
| --- | --- |
| `src/graph.h`, `src/graph.c` | Grafo con listas de adyacencia y generador de grafos aleatorios conexos |
| `src/measure.h`, `src/measure.c` | Reloj monótono y bitácora de las llamadas a `decreaseKey` |
| `src/binomial.h`, `src/binomial.c` | Cola binomial |
| `src/fibonacci.h`, `src/fibonacci.c` | Cola de Fibonacci |
| `src/prim.h`, `src/prim.c` | Las dos versiones del algoritmo de Prim |
| `src/main.c` | Batería de experimentos |
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

## Estado

- [x] Generador de grafos aleatorios conexos
- [x] Infraestructura de medición
- [x] Compilación y pruebas del generador
- [ ] Cola binomial
- [ ] Cola de Fibonacci
- [ ] Prim sobre cada cola
- [ ] Batería de experimentos
