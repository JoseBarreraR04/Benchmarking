#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================================================
#  evaluar.py  -  Parte de EVALUACION del benchmark de convolucion.
#
#  Lee UN SOLO archivo con todas las variantes:
#       <raiz>/resultados.csv
#  (columnas: programa,imagen,ancho,alto,kernel,hilos,repeticiones,
#             media_ms,stddev_ms,min_ms,max_ms,checksum)
#
#  y genera, en la misma raiz:
#       combinado.csv     : las mismas filas + columnas 'speedup' y 'eficiencia'
#       tablas.md         : tablas Markdown listas para pegar en el paper
#       graficas/*.png    : tiempo, speedup y eficiencia vs hilos
#       resumen.txt       : resumen legible por consola
#
#  Metricas:
#       speedup(p)     = T_secuencial / T_p     (misma imagen y kernel)
#       eficiencia(p)  = speedup(p) / hilos
# ============================================================================

import csv
import os
import sys
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

# Orden y etiquetas "bonitas" de los programas.
PROGRAMAS = ["secuencial", "paralelo_ia", "paralelo_ia_opt", "paralelo_manual"]
ETIQUETAS = {
    "secuencial": "Secuencial",
    "paralelo_ia": "Paralelo IA",
    "paralelo_ia_opt": "Paralelo IA + Opt",
    "paralelo_manual": "Paralelo Manual",
}
COLORES = {
    "secuencial": "#444444",
    "paralelo_ia": "#1f77b4",
    "paralelo_ia_opt": "#2ca02c",
    "paralelo_manual": "#d62728",
}


def etiqueta(prog):
    return ETIQUETAS.get(prog, prog)


def color(prog):
    return COLORES.get(prog, "#7f7f7f")


def orden_programas(progs):
    return sorted(progs, key=lambda p: PROGRAMAS.index(p) if p in PROGRAMAS else 99)


# ---------------------------------------------------------------------------
def leer_csv(path):
    filas = []
    with open(path, newline="", encoding="utf-8") as f:
        for d in csv.DictReader(f):
            try:
                filas.append({
                    "programa": d["programa"].strip(),
                    "imagen": d["imagen"].strip(),
                    "ancho": int(d["ancho"]),
                    "alto": int(d["alto"]),
                    "kernel": int(d["kernel"]),
                    "hilos": int(d["hilos"]),
                    "repeticiones": int(d["repeticiones"]),
                    "media_ms": float(d["media_ms"]),
                    "stddev_ms": float(d["stddev_ms"]),
                    "min_ms": float(d["min_ms"]),
                    "max_ms": float(d["max_ms"]),
                    "checksum": d["checksum"].strip(),
                })
            except Exception:
                # Fila de encabezado repetida o linea incompleta: se ignora.
                pass
    return filas


def calcular_metricas(filas):
    """Agrega speedup y eficiencia usando como linea base la version secuencial."""
    base = {}
    for r in filas:
        if r["programa"] == "secuencial" and r["hilos"] == 1:
            base[(r["imagen"], r["kernel"])] = r["media_ms"]
    # Respaldo: si falta la secuencial, se usa el menor tiempo a 1 hilo disponible.
    for r in filas:
        clave = (r["imagen"], r["kernel"])
        if clave not in base and r["hilos"] == 1:
            prev = base.get(clave)
            if prev is None or r["media_ms"] < prev:
                base[clave] = r["media_ms"]
    for r in filas:
        clave = (r["imagen"], r["kernel"])
        b = base.get(clave)
        if b and r["media_ms"] > 0:
            r["speedup"] = b / r["media_ms"]
            r["eficiencia"] = r["speedup"] / r["hilos"] if r["hilos"] else 0.0
        else:
            r["speedup"] = 0.0
            r["eficiencia"] = 0.0
    return filas


def validar(filas):
    """Comprueba que cada version paralela produce el MISMO checksum que la
    secuencial (equivalencia estricta) y que es constante entre hilos."""
    por_checksum = defaultdict(set)
    for r in filas:
        por_checksum[(r["imagen"], r["kernel"], r["programa"])].add(r["checksum"])
    ref = {}
    for r in filas:
        if r["programa"] == "secuencial":
            ref[(r["imagen"], r["kernel"])] = r["checksum"]

    detalle = []
    ok = fallos = 0
    for (img, k, prog), vals in sorted(por_checksum.items()):
        if prog == "secuencial":
            continue
        refcs = ref.get((img, k))
        unico = len(vals) == 1
        coincide = unico and refcs is not None and next(iter(vals)) == refcs
        if coincide:
            ok += 1
        else:
            fallos += 1
        detalle.append({
            "imagen": img, "kernel": k, "programa": prog,
            "checksum": next(iter(vals)) if unico else "VARIOS",
            "referencia": refcs if refcs is not None else "-",
            "ok": coincide,
        })
    return detalle, ok, fallos


# ---------------------------------------------------------------------------
def escribir_combinado(filas, out):
    cols = ["programa", "imagen", "ancho", "alto", "kernel", "hilos",
            "repeticiones", "media_ms", "stddev_ms", "min_ms", "max_ms",
            "checksum", "speedup", "eficiencia"]
    with open(out, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(cols)
        for r in filas:
            w.writerow([
                r["programa"], r["imagen"], r["ancho"], r["alto"],
                r["kernel"], r["hilos"], r["repeticiones"],
                "%.4f" % r["media_ms"], "%.4f" % r["stddev_ms"],
                "%.4f" % r["min_ms"], "%.4f" % r["max_ms"], r["checksum"],
                "%.4f" % r["speedup"], "%.4f" % r["eficiencia"],
            ])


def tabla_md(cabecera, filas):
    lineas = ["| " + " | ".join(cabecera) + " |",
              "|" + "|".join(["---"] * len(cabecera)) + "|"]
    for fila in filas:
        lineas.append("| " + " | ".join(str(c) for c in fila) + " |")
    return "\n".join(lineas)


def tabla_metrica_md(filas, prog, campo, fmt="%.2f"):
    """Tabla (imagen, kernel) x hilos para una metrica ('speedup', 'eficiencia'
    o 'media_ms')."""
    sub = [r for r in filas if r["programa"] == prog]
    if not sub:
        return None
    hilos = sorted({r["hilos"] for r in sub})
    claves = sorted({(r["imagen"], r["kernel"], r["ancho"], r["alto"]) for r in sub},
                    key=lambda t: (t[1], t[3], t[2], t[0]))
    filas_out = []
    for (img, k, w, h) in claves:
        celdas = []
        for hh in hilos:
            vals = [r for r in sub if r["imagen"] == img and r["kernel"] == k and r["hilos"] == hh]
            celdas.append(fmt % vals[0][campo] if vals else "-")
        filas_out.append(["%s (%dx%d)" % (img, w, h), "%dx%d" % (k, k)] + celdas)
    return hilos, tabla_md(["Imagen", "Kernel"] + ["%d h" % hh for hh in hilos], filas_out)


def tabla_agregada_md(filas, prog, campo, fmt="%.2f"):
    """Promedio del campo por kernel (promediando imagenes), x hilos."""
    sub = [r for r in filas if r["programa"] == prog]
    if not sub:
        return None
    hilos = sorted({r["hilos"] for r in sub})
    kernels = sorted({r["kernel"] for r in sub})
    filas_out = []
    for k in kernels:
        celdas = []
        for hh in hilos:
            vals = [r[campo] for r in sub if r["kernel"] == k and r["hilos"] == hh]
            celdas.append(fmt % (sum(vals) / len(vals)) if vals else "-")
        filas_out.append(["%dx%d" % (k, k)] + celdas)
    return tabla_md(["Kernel"] + ["%d h" % hh for hh in hilos], filas_out)


# ---------------------------------------------------------------------------
def graficar(filas, graficas_dir):
    os.makedirs(graficas_dir, exist_ok=True)
    programas = orden_programas({r["programa"] for r in filas})

    def sub(prog, kernel=None):
        s = [r for r in filas if r["programa"] == prog]
        if kernel is not None:
            s = [r for r in s if r["kernel"] == kernel]
        return s

    for prog in programas:
        s = sub(prog)
        if not s:
            continue
        imagenes = sorted({r["imagen"] for r in s})
        kernels = sorted({r["kernel"] for r in s})
        hilos = sorted({r["hilos"] for r in s})
        agregar = len(imagenes) > 6  # resumir cuando hay muchas imagenes

        for campo, titulo, fname in [
            ("media_ms", "Tiempo de convolucion", "tiempo"),
            ("speedup", "Aceleracion (speedup)", "speedup"),
            ("eficiencia", "Eficiencia", "eficiencia"),
        ]:
            fig, axes = plt.subplots(1, len(kernels),
                                     figsize=(4.2 * len(kernels), 3.6),
                                     squeeze=False)
            for idx, k in enumerate(kernels):
                ax = axes[0][idx]
                if agregar:
                    medias, mins, maxs = [], [], []
                    for hh in hilos:
                        vals = [r[campo] for r in s if r["kernel"] == k and r["hilos"] == hh]
                        if vals:
                            medias.append(sum(vals) / len(vals))
                            mins.append(min(vals))
                            maxs.append(max(vals))
                        else:
                            medias.append(float("nan")); mins.append(0); maxs.append(0)
                    ax.plot(hilos, medias, "-o", color=color(prog), label="promedio")
                    ax.fill_between(hilos, mins, maxs, color=color(prog), alpha=0.15,
                                    label="min-max")
                else:
                    for img in imagenes:
                        xs, ys = [], []
                        for hh in hilos:
                            vals = [r[campo] for r in s
                                    if r["imagen"] == img and r["kernel"] == k and r["hilos"] == hh]
                            if vals:
                                xs.append(hh); ys.append(vals[0])
                        ax.plot(xs, ys, "-o", ms=3, label=img)
                ax.set_title("Kernel %dx%d" % (k, k))
                ax.set_xlabel("Hilos")
                ax.set_ylabel(titulo)
                ax.grid(True, alpha=0.3)
                if campo == "speedup":
                    ax.plot(hilos, hilos, "--", color="gray", lw=1, label="lineal")
                if campo == "eficiencia":
                    ax.axhline(1.0, ls="--", color="gray", lw=1)
                ax.legend(fontsize=7)
            fig.suptitle("%s - %s" % (etiqueta(prog), titulo), fontsize=11)
            fig.tight_layout(rect=[0, 0, 1, 0.94])
            fig.savefig(os.path.join(graficas_dir, "%s_%s.png" % (fname, prog)), dpi=130)
            plt.close(fig)

    # ---- Comparacion entre programas sobre la imagen mas grande ----
    if not filas:
        return
    mayor = max({(r["ancho"] * r["alto"], r["imagen"]) for r in filas})[1]
    kernels = sorted({r["kernel"] for r in filas})
    hilos = sorted({r["hilos"] for r in filas})
    for campo, titulo, fname in [("speedup", "Aceleracion (speedup)", "comparacion_speedup"),
                                 ("media_ms", "Tiempo (ms)", "comparacion_tiempo")]:
        fig, axes = plt.subplots(1, len(kernels), figsize=(4.2 * len(kernels), 3.6),
                                 squeeze=False)
        for idx, k in enumerate(kernels):
            ax = axes[0][idx]
            for prog in programas:
                xs, ys = [], []
                for hh in hilos:
                    vals = [r[campo] for r in filas
                            if r["programa"] == prog and r["imagen"] == mayor
                            and r["kernel"] == k and r["hilos"] == hh]
                    if vals:
                        xs.append(hh); ys.append(vals[0])
                if xs:
                    ax.plot(xs, ys, "-o", ms=3, color=color(prog), label=etiqueta(prog))
            ax.set_title("Kernel %dx%d" % (k, k))
            ax.set_xlabel("Hilos"); ax.set_ylabel(titulo)
            ax.grid(True, alpha=0.3); ax.legend(fontsize=7)
        fig.suptitle("%s - imagen %s" % (titulo, mayor), fontsize=11)
        fig.tight_layout(rect=[0, 0, 1, 0.94])
        fig.savefig(os.path.join(graficas_dir, "%s.png" % fname), dpi=130)
        plt.close(fig)


# ---------------------------------------------------------------------------
def main():
    raiz = sys.argv[1] if len(sys.argv) > 1 else "/resultados"
    # Carpeta donde se escriben las salidas (combinado.csv, tablas.md, ...).
    out_dir = raiz if os.path.isdir(raiz) else os.path.dirname(raiz) or "."
    csv_path = raiz if os.path.isfile(raiz) else os.path.join(out_dir, "resultados.csv")

    if os.path.isfile(csv_path):
        # Modo normal: un UNICO archivo con todas las variantes.
        filas = leer_csv(csv_path)
    else:
        # Modo manual: no hay 'resultados.csv'; se combinan todos los *.csv de la
        # carpeta (cada uno producido por un programa distinto).
        filas = []
        for nombre in sorted(os.listdir(out_dir)):
            if nombre.lower().endswith(".csv") and nombre != "combinado.csv":
                filas.extend(leer_csv(os.path.join(out_dir, nombre)))
        if not filas:
            print("[eval] No se encontro 'resultados.csv' ni CSV por programa en %s" % out_dir)
            return 1
    if not filas:
        print("[eval] No se encontraron filas en %s" % csv_path)
        return 1

    filas = calcular_metricas(filas)
    detalle, ok, fallos = validar(filas)
    programas = orden_programas({r["programa"] for r in filas})

    escribir_combinado(filas, os.path.join(out_dir, "combinado.csv"))
    graficar(filas, os.path.join(out_dir, "graficas"))

    # ---------------- Markdown ----------------
    md = []
    md.append("# Resultados del benchmark de convolucion")
    md.append("")
    md.append("Conjunto de imagenes: %s."
              % ", ".join(sorted({"%s (%dx%d)" % (r["imagen"], r["ancho"], r["alto"])
                                  for r in filas})))
    md.append("Metricas por configuracion (media de %d repeticiones, solo convolucion):"
              % (filas[0]["repeticiones"] if filas else 0))
    md.append("- **speedup(p) = T_secuencial / T_p** (misma imagen y kernel)")
    md.append("- **eficiencia(p) = speedup(p) / hilos**")
    md.append("- `h` = numero de hilos. Programas: %s."
              % ", ".join("**%s**" % etiqueta(p) for p in programas))
    md.append("")

    # 1) Validacion
    md.append("## 1. Validacion de equivalencia estricta (checksum FNV-1a)")
    md.append("")
    total = ok + fallos
    if fallos == 0:
        md.append("**%d/%d configuraciones coinciden bit a bit con la version secuencial.** "
                  "La salida es identica, por lo que la comparacion de tiempos es valida."
                  % (ok, total))
    else:
        md.append("**%d/%d coinciden; %d NO coinciden** (revisar la tabla de discrepancias)."
                  % (ok, total, fallos))
        md.append("")
        malas = [d for d in detalle if not d["ok"]]
        md.append(tabla_md(
            ["Imagen", "Kernel", "Programa", "Checksum", "Referencia"],
            [[d["imagen"], "%dx%d" % (d["kernel"], d["kernel"]),
              etiqueta(d["programa"]), d["checksum"], d["referencia"]] for d in malas[:50]]))
    md.append("")

    # 2) Tablas por programa
    for prog in programas:
        filas_prog = [r for r in filas if r["programa"] == prog]
        if not filas_prog:
            continue
        n_imgs = len({r["imagen"] for r in filas_prog})
        detallado = n_imgs <= 8
        md.append("## %s" % etiqueta(prog))
        md.append("")

        if prog != "secuencial":
            if detallado:
                res = tabla_metrica_md(filas, prog, "speedup")
                if res:
                    md.append("**Aceleracion (speedup) vs hilos**")
                    md.append("")
                    md.append(res[1])
                    md.append("")
                res = tabla_metrica_md(filas, prog, "eficiencia")
                if res:
                    md.append("**Eficiencia vs hilos**")
                    md.append("")
                    md.append(res[1])
                    md.append("")
            else:
                md.append("**Speedup promedio por kernel (%d imagenes) vs hilos**" % n_imgs)
                md.append("")
                md.append(tabla_agregada_md(filas, prog, "speedup") or "-")
                md.append("")
                md.append("**Eficiencia promedio por kernel (%d imagenes) vs hilos**" % n_imgs)
                md.append("")
                md.append(tabla_agregada_md(filas, prog, "eficiencia") or "-")
                md.append("")

        if detallado:
            res = tabla_metrica_md(filas, prog, "media_ms", fmt="%.2f")
            if res:
                md.append("**Tiempo medio (ms) vs hilos**")
                md.append("")
                md.append(res[1])
                md.append("")
        else:
            md.append("**Tiempo medio (ms) promedio por kernel vs hilos**")
            md.append("")
            md.append(tabla_agregada_md(filas, prog, "media_ms", fmt="%.2f") or "-")
            md.append("")

    md.append("## Figuras")
    md.append("")
    md.append("Disponibles en `graficas/`: tiempo, speedup y eficiencia vs hilos por "
              "programa, ademas de la comparacion directa entre programas sobre la "
              "imagen mas grande.")
    md.append("")

    with open(os.path.join(out_dir, "tablas.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(md))

    resumen = [
        "Conjunto: %s imagenes" % len({r["imagen"] for r in filas}),
        "Programas: %s" % ", ".join(etiqueta(p) for p in programas),
        "Validacion estricta: %d/%d coinciden, %d fallos" % (ok, total, fallos),
        "Generado: combinado.csv, tablas.md, graficas/*.png",
    ]
    with open(os.path.join(out_dir, "resumen.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(resumen) + "\n")
    print("\n".join(resumen))
    return 0


if __name__ == "__main__":
    sys.exit(main())
