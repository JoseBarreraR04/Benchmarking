#!/usr/bin/env bash
# ============================================================================
#  run_all.sh
#  Ejecuta TODAS las variantes del benchmark sobre UN SOLO conjunto de imagenes
#  (por defecto /app/Images: 512, 1024, 2048 y 4096 px) y deja toda la
#  informacion en UN SOLO archivo CSV, ademas de las tablas y graficas.
#
#  Variables de entorno opcionales:
#    BENCH_REPS      Repeticiones cronometradas por configuracion (por defecto 30)
#    BENCH_DATASET   Carpeta de imagenes a usar (por defecto /app/Images)
# ============================================================================
set -uo pipefail

OUT="/resultados"
mkdir -p "$OUT"
# Se parte de cero en cada corrida (nada de resultados viejos mezclados).
rm -f "$OUT"/*.csv "$OUT"/resumen.txt
rm -rf "$OUT/graficas"
mkdir -p "$OUT/graficas"
# Limpieza del esquema antiguo de dos conjuntos (setA/setB), si existiera.
rm -rf "$OUT/setA" "$OUT/setB" "$OUT/manifiesto_setB.txt"

export MPLBACKEND=Agg
export BENCH_REPS="${BENCH_REPS:-30}"
DATASET="${BENCH_DATASET:-/app/Images}"

echo "=========================================================="
echo " Benchmark de convolucion  -  $(nproc) nucleos visibles"
echo " Conjunto de imagenes: ${DATASET}"
echo " Repeticiones por configuracion: ${BENCH_REPS}"
echo " Resultados en: ${OUT}"
echo "=========================================================="

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

# --- UN solo CSV con todas las variantes (encabezado una sola vez) ----------
CSV="$OUT/resultados.csv"
echo "programa,imagen,ancho,alto,kernel,hilos,repeticiones,media_ms,stddev_ms,min_ms,max_ms,checksum" > "$CSV"

for p in "${PROGS[@]}"; do
    echo ">>> ${p} ..."
    # Cada programa imprime su CSV (con encabezado) en stdout; se descarta ese
    # encabezado con 'tail -n +2' y las filas se acumulan en el unico CSV.
    BENCH_DATASET="$DATASET" "/app/${p}" | tail -n +2 >> "$CSV"
    echo "    -> total filas acumuladas: $(($(wc -l < "$CSV") - 1))"
done
echo ""

# --- Evaluacion (tablas + graficas) -----------------------------------------
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
