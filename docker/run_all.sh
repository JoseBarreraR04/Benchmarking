#!/usr/bin/env bash
# ============================================================================
#  run_all.sh - Ejecuta TODAS las variantes del benchmark y genera la
#  evaluacion (CSV combinado, tablas Markdown y graficas).
#
#  Conjuntos de imagenes:
#    setA : /app/Images -> 512, 1024, 2048 y 4096 px (enunciado de la tarea)
#    setB : 30 imagenes aleatorias de /app/imgs (1080x720, variedad de contenido)
#
#  Variables de entorno opcionales:
#    BENCH_REPS      Repeticiones cronometradas por configuracion (por defecto 30)
#    BENCH_DATASETS  Conjuntos a correr, p. ej. "setA" o "setA setB" (por defecto ambos)
# ============================================================================
set -uo pipefail

OUT="/resultados"
mkdir -p "$OUT"
rm -rf "$OUT/setA" "$OUT/setB" "$OUT/graficas"
mkdir -p "$OUT/setA" "$OUT/setB" "$OUT/graficas"

export MPLBACKEND=Agg
export BENCH_REPS="${BENCH_REPS:-30}"
DATASETS="${BENCH_DATASETS:-setA setB}"

echo "=========================================================="
echo " Benchmark de convolucion  -  $(nproc) nucleos visibles"
echo " Repeticiones por configuracion: ${BENCH_REPS}"
echo " Conjuntos: ${DATASETS}"
echo " Resultados en: ${OUT}"
echo "=========================================================="

# --- Conjunto B: 30 imagenes aleatorias de imgs/ ---------------------------
DATASET_B="/app/dataset_b"
rm -rf "$DATASET_B"
mkdir -p "$DATASET_B"
if compgen -G "/app/imgs/*.jpg" > /dev/null; then
    # Seleccion aleatoria (reproducible solo por el manifiesto que se guarda).
    mapfile -t SEL < <(ls /app/imgs/*.jpg | shuf -n 30)
    for f in "${SEL[@]}"; do
        ln -sf "$f" "$DATASET_B/$(basename "$f")"
    done
    printf '%s\n' "${SEL[@]}" | xargs -n1 basename | sort > "$OUT/manifiesto_setB.txt"
    echo "[dataset] setB: ${#SEL[@]} imagenes aleatorias (ver manifiesto_setB.txt)"
else
    echo "[dataset] AVISO: no se encontro /app/imgs; setB se omitira"
    DATASETS="${DATASETS/setB/}"
fi

# --- Programas disponibles --------------------------------------------------
PROGS=()
for p in secuencial paralelo_ia paralelo_ia_opt paralelo_manual; do
    if [ -x "/app/$p" ]; then
        PROGS+=("$p")
    fi
done
if [ "${#PROGS[@]}" -eq 0 ]; then
    echo "[ERROR] No hay ejecutables compilados."
    exit 1
fi
echo "[benchmark] Programas: ${PROGS[*]}"
echo ""

correr_conjunto() {
    local nombre="$1"
    local carpeta="$2"
    echo "==================== Conjunto ${nombre} ===================="
    for p in "${PROGS[@]}"; do
        echo ">>> ${p} (${nombre}) ..."
        BENCH_DATASET="$carpeta" "/app/${p}" > "${OUT}/${nombre}/${p}.csv"
        echo "    -> ${OUT}/${nombre}/${p}.csv"
    done
    echo ""
}

for ds in ${DATASETS}; do
    if [ "$ds" = "setA" ]; then
        if [ -d "/app/Images" ]; then
            correr_conjunto setA /app/Images
        else
            echo "[AVISO] No existe /app/Images; setA omitido"
        fi
    elif [ "$ds" = "setB" ]; then
        if [ -n "$(ls -A "$DATASET_B" 2>/dev/null)" ]; then
            correr_conjunto setB "$DATASET_B"
        else
            echo "[AVISO] setB vacio; omitido"
        fi
    fi
done

# --- Evaluacion -------------------------------------------------------------
echo "=========================================================="
echo " Generando evaluacion (combinado.csv, tablas.md, graficas/)"
echo "=========================================================="
if python3 /app/evaluar.py "$OUT"; then
    echo "[eval] Evaluacion generada correctamente."
else
    echo "[eval] AVISO: fallo la generacion de tablas/graficas."
fi

echo ""
echo "=========================================================="
echo " Benchmark finalizado. Archivos en ${OUT}:"
ls -1 "$OUT"
echo "=========================================================="
