#!/usr/bin/env python3
"""Post-procesa las mediciones de results/ y genera los doce graficos pedidos,
la tabla de tiempos promedio y la verificacion de que ambas colas producen un
MST del mismo peso.

Entradas (las escribe build/tarea1 usando src/report.c):
  results/runs.csv
      una fila por corrida, con columnas
      serie,queue,i,j,rep,v,e,mst_weight,mst_edges,total_time,
      extract_min_calls,decrease_key_calls,decrease_key_ops,decrease_key_time
  results/decrease_<cola>_<serie>_i<i>_j<j>_r<rep>.csv
      curva acumulada de decreaseKey, con columnas calls,cum_seconds,cum_ops

Salidas, en figures/:
  total_<cola>_serie<A|B>.png            (4 graficos, seccion 6.3.1)
  decrease_<time|ops>_<cola>_serie<C|D>.png  (8 graficos, seccion 6.3.2)
  tabla_tiempos.csv y tabla_tiempos.md
"""

import argparse
import csv
import math
import os
import re
import sys
from collections import defaultdict

import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

QUEUES = ("binomial", "fibonacci")

# Para cada serie, que parametro varia y como se rotula el eje x.
SERIES_AXIS = {
    "A": ("j", "aristas  e = 2^j"),
    "B": ("i", "vertices  v = 2^i"),
    "C": ("j", "aristas  e = 2^j"),
    "D": ("i", "vertices  v = 2^i"),
}

QUEUE_LABEL = {"binomial": "cola binomial", "fibonacci": "cola de Fibonacci"}

CURVE_NAME = re.compile(
    r"^decrease_(?P<queue>\w+)_(?P<serie>[A-D])_i(?P<i>\d+)_j(?P<j>\d+)_r(?P<rep>\d+)\.csv$"
)


# --- Lectura de datos -------------------------------------------------------


def read_runs(path):
    """Lee results/runs.csv y convierte las columnas numericas."""
    if not os.path.exists(path):
        sys.exit(f"No existe {path}. Corre primero ./build/tarea1.")

    ints = ("i", "j", "rep", "v", "e", "mst_edges", "extract_min_calls",
            "decrease_key_calls", "decrease_key_ops")
    floats = ("mst_weight", "total_time", "decrease_key_time")

    rows = []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            for key in ints:
                row[key] = int(row[key])
            for key in floats:
                row[key] = float(row[key])
            rows.append(row)

    if not rows:
        sys.exit(f"{path} no tiene filas.")
    return rows


def read_curves(results_dir):
    """Agrupa las curvas de decreaseKey por (cola, serie, i, j)."""
    curves = defaultdict(list)
    if not os.path.isdir(results_dir):
        return curves

    for name in sorted(os.listdir(results_dir)):
        match = CURVE_NAME.match(name)
        if match is None:
            continue
        calls, seconds, ops = [], [], []
        with open(os.path.join(results_dir, name), newline="") as f:
            for row in csv.DictReader(f):
                calls.append(int(row["calls"]))
                seconds.append(float(row["cum_seconds"]))
                ops.append(int(row["cum_ops"]))
        if not calls:
            continue
        key = (match["queue"], match["serie"], int(match["i"]), int(match["j"]))
        curves[key].append(
            (np.array(calls, dtype=float), np.array(seconds), np.array(ops, dtype=float))
        )
    return curves


# --- Cotas teoricas y ajuste de constantes ----------------------------------


def total_bound(queue, v, e):
    """Cota del tiempo total: O(e log v) para la binomial, O(e + v log v) para
    la de Fibonacci."""
    if queue == "binomial":
        return e * math.log2(v)
    return e + v * math.log2(v)


def decrease_bound(queue, calls, v):
    """Cota del costo acumulado de k llamadas a decreaseKey: O(k log v) en la
    binomial (peor caso por llamada) y O(k) en la de Fibonacci (constante
    amortizado)."""
    if queue == "binomial":
        return calls * math.log2(v)
    return calls.astype(float) if hasattr(calls, "astype") else float(calls)


def fit_constant(measured, bound):
    """Constante c que minimiza el error cuadratico de c*bound contra measured.

    Se ajusta una recta por el origen porque las cotas son asintoticas y solo
    quedan determinadas hasta un factor multiplicativo.
    """
    measured = np.asarray(measured, dtype=float)
    bound = np.asarray(bound, dtype=float)
    denominator = float(np.dot(bound, bound))
    if denominator == 0.0:
        return 0.0
    return float(np.dot(measured, bound)) / denominator


# --- Graficos de tiempo total (seccion 6.3.1) -------------------------------


def average_totals(runs, serie, queue):
    """Promedia el tiempo total de las repeticiones de cada configuracion."""
    grouped = defaultdict(list)
    for row in runs:
        if row["serie"] == serie and row["queue"] == queue:
            grouped[(row["i"], row["j"])].append(row["total_time"])

    axis = SERIES_AXIS[serie][0]
    configs = sorted(grouped, key=lambda key: key[0] if axis == "i" else key[1])
    return [
        {
            "i": i,
            "j": j,
            "x": i if axis == "i" else j,
            "mean": float(np.mean(grouped[(i, j)])),
            "std": float(np.std(grouped[(i, j)])),
            "reps": len(grouped[(i, j)]),
        }
        for (i, j) in configs
    ]


def plot_total(serie, queue, points, ylim, out_dir):
    x = np.array([p["x"] for p in points], dtype=float)
    measured = np.array([p["mean"] for p in points])
    errors = np.array([p["std"] for p in points])
    bound = np.array([total_bound(queue, 2.0 ** p["i"], 2.0 ** p["j"]) for p in points])
    c = fit_constant(measured, bound)

    formula = "e log v" if queue == "binomial" else "e + v log v"
    figure, axes = plt.subplots(figsize=(6.4, 4.4))
    axes.errorbar(x, measured, yerr=errors, marker="o", capsize=3,
                  label=f"medido ({QUEUE_LABEL[queue]})")
    axes.plot(x, c * bound, linestyle="--", marker="x",
              label=f"cota teorica  {c:.3g} * ({formula})")

    axes.set_xlabel(f"exponente de la serie {serie}: {SERIES_AXIS[serie][1]}")
    axes.set_ylabel("tiempo total [s]")
    axes.set_title(f"Tiempo total, serie {serie}, {QUEUE_LABEL[queue]}")
    axes.set_xticks(x)
    axes.set_ylim(0, ylim)
    axes.grid(alpha=0.3)
    axes.legend()
    figure.tight_layout()

    path = os.path.join(out_dir, f"total_{queue}_serie{serie}.png")
    figure.savefig(path, dpi=150)
    plt.close(figure)
    return path, c


# --- Graficos de costo amortizado (seccion 6.3.2) ---------------------------


def average_curves(runs_of_config, column, points=200):
    """Promedia las curvas de varias repeticiones interpolando sobre una grilla
    comun de cantidad de llamadas.

    Las repeticiones no hacen la misma cantidad de llamadas (el grafo cambia en
    cada una), asi que no se pueden promediar punto a punto: se recorta al minimo
    comun y se interpola.
    """
    index = 1 if column == "seconds" else 2
    limit = min(curve[0][-1] for curve in runs_of_config)
    grid = np.linspace(limit / points, limit, points)
    stacked = [np.interp(grid, curve[0], curve[index]) for curve in runs_of_config]
    return grid, np.mean(stacked, axis=0)


def plot_decrease(serie, queue, measure, curves, ylim, out_dir):
    """Un grafico por (serie, cola, medicion), con una curva por configuracion."""
    keys = sorted(
        (key for key in curves if key[0] == queue and key[1] == serie),
        key=lambda key: key[2] if SERIES_AXIS[serie][0] == "i" else key[3],
    )
    if not keys:
        return None, {}

    column = "seconds" if measure == "time" else "ops"
    unit = "tiempo acumulado [s]" if measure == "time" else (
        "intercambios acumulados" if queue == "binomial" else "cortes acumulados"
    )
    formula = "k log v" if queue == "binomial" else "k"

    figure, axes = plt.subplots(figsize=(6.4, 4.4))
    constants = {}

    for index, key in enumerate(keys):
        _, _, i, j = key
        grid, mean = average_curves(curves[key], column)
        bound = decrease_bound(queue, grid, 2.0 ** i)
        c = fit_constant(mean, bound)
        constants[(i, j)] = c
        color = f"C{index}"
        axes.plot(grid, mean, color=color, label=f"medido  v=2^{i}, e=2^{j}")
        axes.plot(grid, c * bound, color=color, linestyle="--", linewidth=1,
                  label=f"cota  {c:.3g} * ({formula})")

    axes.set_xlabel("llamadas a decreaseKey")
    axes.set_ylabel(unit)
    axes.set_title(f"decreaseKey acumulado ({'tiempo' if measure == 'time' else 'operaciones'}), "
                   f"serie {serie}, {QUEUE_LABEL[queue]}")
    axes.set_ylim(0, ylim)
    axes.grid(alpha=0.3)
    axes.legend(fontsize=7, ncol=2)
    figure.tight_layout()

    path = os.path.join(out_dir, f"decrease_{measure}_{queue}_serie{serie}.png")
    figure.savefig(path, dpi=150)
    plt.close(figure)
    return path, constants


def decrease_ylim(serie, measure, curves):
    """Maximo comun a las dos colas, para que los graficos de una misma serie y
    medicion queden en la misma escala y se puedan comparar."""
    column = "seconds" if measure == "time" else "ops"
    peak = 0.0
    for queue in QUEUES:
        for key in curves:
            if key[0] == queue and key[1] == serie:
                _, mean = average_curves(curves[key], column)
                peak = max(peak, float(mean[-1]))
    return peak * 1.08 if peak > 0 else 1.0


# --- Tabla y verificacion ---------------------------------------------------


def write_table(runs, out_dir):
    """Tabla de tiempos por configuracion con el promedio de las repeticiones."""
    grouped = defaultdict(dict)
    for row in runs:
        grouped[(row["serie"], row["i"], row["j"])].setdefault(row["queue"], []).append(
            row["total_time"]
        )

    rows = []
    for (serie, i, j) in sorted(grouped):
        entry = grouped[(serie, i, j)]
        binomial = entry.get("binomial", [])
        fibonacci = entry.get("fibonacci", [])
        rows.append({
            "serie": serie,
            "i": i,
            "j": j,
            "v": 2 ** i,
            "e": 2 ** j,
            "reps": max(len(binomial), len(fibonacci)),
            "binomial_s": float(np.mean(binomial)) if binomial else float("nan"),
            "binomial_std": float(np.std(binomial)) if binomial else float("nan"),
            "fibonacci_s": float(np.mean(fibonacci)) if fibonacci else float("nan"),
            "fibonacci_std": float(np.std(fibonacci)) if fibonacci else float("nan"),
        })

    csv_path = os.path.join(out_dir, "tabla_tiempos.csv")
    with open(csv_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)

    md_path = os.path.join(out_dir, "tabla_tiempos.md")
    with open(md_path, "w") as f:
        f.write("| Serie | v | e | Reps | Binomial [s] | Fibonacci [s] | Razon |\n")
        f.write("| --- | --- | --- | --- | --- | --- | --- |\n")
        for row in rows:
            ratio = (row["binomial_s"] / row["fibonacci_s"]
                     if row["fibonacci_s"] and not math.isnan(row["fibonacci_s"]) else float("nan"))
            f.write(f"| {row['serie']} | 2^{row['i']} | 2^{row['j']} | {row['reps']} "
                    f"| {row['binomial_s']:.4f} ± {row['binomial_std']:.4f} "
                    f"| {row['fibonacci_s']:.4f} ± {row['fibonacci_std']:.4f} "
                    f"| {ratio:.2f}x |\n")

    return csv_path, md_path, rows


def verify_weights(runs, tolerance=1e-9):
    """Comprueba que las dos colas entreguen un MST del mismo peso en cada
    corrida. El MST no es unico si hay pesos repetidos, pero su peso total si.
    """
    grouped = defaultdict(dict)
    for row in runs:
        grouped[(row["serie"], row["i"], row["j"], row["rep"])][row["queue"]] = row

    compared = 0
    mismatches = []
    worst = 0.0
    incomplete = []

    for key in sorted(grouped):
        entry = grouped[key]
        if len(entry) < 2:
            continue
        compared += 1
        difference = abs(entry["binomial"]["mst_weight"] - entry["fibonacci"]["mst_weight"])
        worst = max(worst, difference)
        if difference > tolerance:
            mismatches.append((key, difference))
        for queue in QUEUES:
            row = entry[queue]
            if row["mst_edges"] != row["v"] - 1:
                incomplete.append((key, queue, row["mst_edges"], row["v"] - 1))

    return compared, worst, mismatches, incomplete


# --- Programa principal -----------------------------------------------------


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--results", default="results", help="directorio con los CSV de entrada")
    parser.add_argument("--out", default="figures", help="directorio de salida")
    args = parser.parse_args()

    os.makedirs(args.out, exist_ok=True)
    runs = read_runs(os.path.join(args.results, "runs.csv"))
    curves = read_curves(args.results)

    print(f"{len(runs)} corridas leidas, {len(curves)} configuraciones con curva de decreaseKey\n")

    # Cuatro graficos de tiempo total, con escala compartida dentro de cada serie.
    print("Tiempo total (seccion 6.3.1)")
    for serie in ("A", "B"):
        points = {queue: average_totals(runs, serie, queue) for queue in QUEUES}
        if not any(points.values()):
            print(f"  serie {serie}: sin datos")
            continue
        peak = max((p["mean"] + p["std"] for values in points.values() for p in values), default=1.0)
        for queue in QUEUES:
            if not points[queue]:
                print(f"  serie {serie}, {queue}: sin datos")
                continue
            path, c = plot_total(serie, queue, points[queue], peak * 1.08, args.out)
            print(f"  {path}  (constante ajustada: {c:.4g})")

    # Ocho graficos de costo amortizado, con escala compartida por serie y medicion.
    print("\nCosto amortizado de decreaseKey (seccion 6.3.2)")
    for serie in ("C", "D"):
        for measure in ("time", "ops"):
            ylim = decrease_ylim(serie, measure, curves)
            for queue in QUEUES:
                path, constants = plot_decrease(serie, queue, measure, curves, ylim, args.out)
                if path is None:
                    print(f"  serie {serie}, {measure}, {queue}: sin datos")
                    continue
                shown = ", ".join(f"v=2^{i} e=2^{j}: {c:.3g}"
                                  for (i, j), c in sorted(constants.items()))
                print(f"  {path}\n      constantes ajustadas -> {shown}")

    print("\nTabla de tiempos")
    csv_path, md_path, _ = write_table(runs, args.out)
    print(f"  {csv_path}\n  {md_path}")

    print("\nVerificacion de que ambas colas dan el mismo peso de MST")
    compared, worst, mismatches, incomplete = verify_weights(runs)
    print(f"  configuraciones comparadas: {compared}")
    print(f"  diferencia absoluta maxima: {worst:.3g}")
    if mismatches:
        print(f"  ATENCION: {len(mismatches)} corridas difieren")
        for key, difference in mismatches[:10]:
            serie, i, j, rep = key
            print(f"    serie {serie} v=2^{i} e=2^{j} rep {rep}: diferencia {difference:.3g}")
    else:
        print("  todas las corridas coinciden")
    if incomplete:
        print(f"  ATENCION: {len(incomplete)} corridas no produjeron un arbol cobertor completo")
        for key, queue, got, expected in incomplete[:10]:
            serie, i, j, rep = key
            print(f"    serie {serie} v=2^{i} e=2^{j} rep {rep} ({queue}): "
                  f"{got} aristas en vez de {expected}")

    return 1 if mismatches or incomplete else 0


if __name__ == "__main__":
    sys.exit(main())
