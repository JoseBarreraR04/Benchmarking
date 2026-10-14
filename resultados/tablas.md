# Resultados del benchmark de convolucion

Metricas por configuracion (media de 30 repeticiones, solo convolucion):
- **speedup(p) = T_secuencial / T_p** (misma imagen y kernel)
- **eficiencia(p) = speedup(p) / hilos**
- `h` = numero de hilos. Programas: **Secuencial**, **Paralelo IA**.

## 1. Validacion de equivalencia estricta (checksum FNV-1a)

**136/136 configuraciones coinciden bit a bit con la version secuencial.** La salida es identica, por lo que la comparacion de tiempos es valida.

## Conjunto `setA`

### Secuencial

**Tiempo medio (ms) vs hilos**

| Imagen | Kernel | 1 h |
|---|---|---|
| Avion512.png (512x512) | 3x3 | 9.71 |
| Male1024.png (1024x1024) | 3x3 | 36.78 |
| nube2048.png (2048x2048) | 3x3 | 146.54 |
| minecraft4096.png (4096x4096) | 3x3 | 573.55 |
| Avion512.png (512x512) | 5x5 | 20.59 |
| Male1024.png (1024x1024) | 5x5 | 88.02 |
| nube2048.png (2048x2048) | 5x5 | 319.15 |
| minecraft4096.png (4096x4096) | 5x5 | 1251.18 |
| Avion512.png (512x512) | 7x7 | 36.34 |
| Male1024.png (1024x1024) | 7x7 | 146.16 |
| nube2048.png (2048x2048) | 7x7 | 571.04 |
| minecraft4096.png (4096x4096) | 7x7 | 2253.52 |
| Avion512.png (512x512) | 11x11 | 84.94 |
| Male1024.png (1024x1024) | 11x11 | 338.29 |
| nube2048.png (2048x2048) | 11x11 | 1383.06 |
| minecraft4096.png (4096x4096) | 11x11 | 5654.05 |

### Paralelo IA

**Aceleracion (speedup) vs hilos**

| Imagen | Kernel | 1 h | 2 h | 4 h | 8 h | 16 h |
|---|---|---|---|---|---|---|
| Avion512.png (512x512) | 3x3 | 1.02 | 1.83 | 2.47 | 1.54 | 3.10 |
| Male1024.png (1024x1024) | 3x3 | 1.00 | 1.88 | 3.16 | 3.72 | 4.53 |
| nube2048.png (2048x2048) | 3x3 | 0.94 | 1.82 | 2.92 | 3.61 | 4.76 |
| minecraft4096.png (4096x4096) | 3x3 | 0.97 | 1.87 | 3.07 | 4.11 | 5.03 |
| Avion512.png (512x512) | 5x5 | 1.01 | 2.01 | 3.45 | 3.83 | 4.09 |
| Male1024.png (1024x1024) | 5x5 | 1.09 | 2.04 | 3.47 | 4.39 | 5.79 |
| nube2048.png (2048x2048) | 5x5 | 0.97 | 1.72 | 2.76 | 3.63 | 3.87 |
| minecraft4096.png (4096x4096) | 5x5 | 0.97 | 1.92 | 3.06 | 4.51 | 5.37 |
| Avion512.png (512x512) | 7x7 | 1.00 | 1.96 | 3.74 | 3.94 | 4.56 |
| Male1024.png (1024x1024) | 7x7 | 1.00 | 1.93 | 2.99 | 4.12 | 5.18 |
| nube2048.png (2048x2048) | 7x7 | 0.96 | 1.69 | 3.02 | 4.39 | 5.10 |
| minecraft4096.png (4096x4096) | 7x7 | 0.98 | 1.92 | 3.13 | 4.60 | 5.38 |
| Avion512.png (512x512) | 11x11 | 1.00 | 1.95 | 3.29 | 4.08 | 5.30 |
| Male1024.png (1024x1024) | 11x11 | 0.98 | 1.90 | 3.10 | 4.28 | 5.37 |
| nube2048.png (2048x2048) | 11x11 | 0.98 | 1.98 | 2.93 | 4.38 | 3.91 |
| minecraft4096.png (4096x4096) | 11x11 | 1.01 | 2.01 | 3.24 | 4.75 | 4.08 |

**Eficiencia vs hilos**

| Imagen | Kernel | 1 h | 2 h | 4 h | 8 h | 16 h |
|---|---|---|---|---|---|---|
| Avion512.png (512x512) | 3x3 | 1.02 | 0.91 | 0.62 | 0.19 | 0.19 |
| Male1024.png (1024x1024) | 3x3 | 1.00 | 0.94 | 0.79 | 0.46 | 0.28 |
| nube2048.png (2048x2048) | 3x3 | 0.94 | 0.91 | 0.73 | 0.45 | 0.30 |
| minecraft4096.png (4096x4096) | 3x3 | 0.97 | 0.93 | 0.77 | 0.51 | 0.31 |
| Avion512.png (512x512) | 5x5 | 1.01 | 1.00 | 0.86 | 0.48 | 0.26 |
| Male1024.png (1024x1024) | 5x5 | 1.09 | 1.02 | 0.87 | 0.55 | 0.36 |
| nube2048.png (2048x2048) | 5x5 | 0.97 | 0.86 | 0.69 | 0.45 | 0.24 |
| minecraft4096.png (4096x4096) | 5x5 | 0.97 | 0.96 | 0.77 | 0.56 | 0.34 |
| Avion512.png (512x512) | 7x7 | 1.00 | 0.98 | 0.94 | 0.49 | 0.29 |
| Male1024.png (1024x1024) | 7x7 | 1.00 | 0.97 | 0.75 | 0.51 | 0.32 |
| nube2048.png (2048x2048) | 7x7 | 0.96 | 0.85 | 0.75 | 0.55 | 0.32 |
| minecraft4096.png (4096x4096) | 7x7 | 0.98 | 0.96 | 0.78 | 0.58 | 0.34 |
| Avion512.png (512x512) | 11x11 | 1.00 | 0.98 | 0.82 | 0.51 | 0.33 |
| Male1024.png (1024x1024) | 11x11 | 0.98 | 0.95 | 0.77 | 0.54 | 0.34 |
| nube2048.png (2048x2048) | 11x11 | 0.98 | 0.99 | 0.73 | 0.55 | 0.24 |
| minecraft4096.png (4096x4096) | 11x11 | 1.01 | 1.01 | 0.81 | 0.59 | 0.26 |

**Tiempo medio (ms) vs hilos**

| Imagen | Kernel | 1 h | 2 h | 4 h | 8 h | 16 h |
|---|---|---|---|---|---|---|
| Avion512.png (512x512) | 3x3 | 9.55 | 5.31 | 3.93 | 6.32 | 3.13 |
| Male1024.png (1024x1024) | 3x3 | 36.83 | 19.60 | 11.62 | 9.89 | 8.12 |
| nube2048.png (2048x2048) | 3x3 | 156.31 | 80.62 | 50.12 | 40.61 | 30.79 |
| minecraft4096.png (4096x4096) | 3x3 | 591.46 | 306.78 | 186.57 | 139.67 | 114.11 |
| Avion512.png (512x512) | 5x5 | 20.29 | 10.25 | 5.96 | 5.37 | 5.03 |
| Male1024.png (1024x1024) | 5x5 | 80.44 | 43.08 | 25.38 | 20.03 | 15.20 |
| nube2048.png (2048x2048) | 5x5 | 330.55 | 185.97 | 115.72 | 87.91 | 82.52 |
| minecraft4096.png (4096x4096) | 5x5 | 1293.02 | 651.10 | 408.31 | 277.33 | 233.14 |
| Avion512.png (512x512) | 7x7 | 36.26 | 18.51 | 9.70 | 9.22 | 7.96 |
| Male1024.png (1024x1024) | 7x7 | 146.83 | 75.63 | 48.84 | 35.50 | 28.21 |
| nube2048.png (2048x2048) | 7x7 | 595.54 | 337.01 | 189.21 | 130.03 | 111.94 |
| minecraft4096.png (4096x4096) | 7x7 | 2304.04 | 1175.68 | 719.47 | 489.63 | 419.06 |
| Avion512.png (512x512) | 11x11 | 85.15 | 43.55 | 25.85 | 20.84 | 16.04 |
| Male1024.png (1024x1024) | 11x11 | 345.01 | 177.95 | 109.17 | 78.97 | 62.97 |
| nube2048.png (2048x2048) | 11x11 | 1411.13 | 698.64 | 471.74 | 315.95 | 353.33 |
| minecraft4096.png (4096x4096) | 11x11 | 5599.42 | 2808.94 | 1744.69 | 1191.40 | 1385.62 |

## Conjunto `setB`

### Secuencial

**Tiempo medio (ms) promedio por kernel vs hilos**

| Kernel | 1 h |
|---|---|
| 3x3 | 27.72 |
| 5x5 | 60.54 |
| 7x7 | 109.99 |
| 11x11 | 264.01 |

### Paralelo IA

**Speedup promedio por kernel (30 imagenes) vs hilos**

| Kernel | 1 h | 2 h | 4 h | 8 h | 16 h |
|---|---|---|---|---|---|
| 3x3 | 0.99 | 1.87 | 2.84 | 3.60 | 4.13 |
| 5x5 | 1.00 | 1.90 | 2.99 | 3.95 | 4.78 |
| 7x7 | 1.01 | 1.95 | 3.02 | 4.03 | 5.07 |
| 11x11 | 1.01 | 1.97 | 3.10 | 4.38 | 5.32 |

**Eficiencia promedio por kernel (30 imagenes) vs hilos**

| Kernel | 1 h | 2 h | 4 h | 8 h | 16 h |
|---|---|---|---|---|---|
| 3x3 | 0.99 | 0.94 | 0.71 | 0.45 | 0.26 |
| 5x5 | 1.00 | 0.95 | 0.75 | 0.49 | 0.30 |
| 7x7 | 1.01 | 0.97 | 0.75 | 0.50 | 0.32 |
| 11x11 | 1.01 | 0.98 | 0.77 | 0.55 | 0.33 |

**Tiempo medio (ms) promedio por kernel vs hilos**

| Kernel | 1 h | 2 h | 4 h | 8 h | 16 h |
|---|---|---|---|---|---|
| 3x3 | 28.17 | 14.93 | 9.96 | 7.75 | 6.74 |
| 5x5 | 60.87 | 32.11 | 20.60 | 15.39 | 12.76 |
| 7x7 | 108.99 | 56.75 | 36.70 | 27.34 | 21.88 |
| 11x11 | 261.03 | 134.45 | 85.33 | 60.44 | 50.21 |

## Figuras

Disponibles en `graficas/`: tiempo, speedup y eficiencia vs hilos por programa y conjunto, ademas de la comparacion directa entre programas sobre la imagen mas grande.
