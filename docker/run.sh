#!/usr/bin/env bash
# Ejecuta ambas versiones del benchmark y guarda los CSV en /resultados.
set -euo pipefail

OUT="/resultados"
mkdir -p "$OUT"

echo "=========================================================="
echo " Benchmark de convolucion - $(nproc) hilos visibles"
echo " Resultados en: $OUT"
echo "=========================================================="

echo ">>> Ejecutando version SECUENCIAL ..."
./secuencial > "$OUT/secuencial.csv"
echo ">>> Listo: $OUT/secuencial.csv"

echo ">>> Ejecutando version PARALELA (OpenMP) ..."
./paralelo_ia > "$OUT/paralelo_ia.csv"
echo ">>> Listo: $OUT/paralelo_ia.csv"

echo "=========================================================="
echo " Benchmark finalizado. CSV disponibles en $OUT"
echo "=========================================================="
