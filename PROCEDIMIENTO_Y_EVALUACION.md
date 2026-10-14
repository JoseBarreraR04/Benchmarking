# Procedimiento y Evaluación

> Sección lista para el paper. Las tablas numéricas y las figuras se generan
> automáticamente en `resultados/tablas.md` y `resultados/graficas/` cada vez que
> se ejecuta el benchmark en Docker.

## 1. Objetivo

Comparar, bajo **idénticas condiciones experimentales**, el rendimiento de una
implementación **secuencial** de convolución de imágenes frente a variantes
**paralelas** (generadas por IA y escrita a mano), midiendo el tiempo de cómputo,
la **aceleración (speedup)** y la **eficiencia** conforme varían el tamaño de la
imagen, el tamaño del kernel y el número de hilos.

## 2. Programas comparados

| Programa | Origen | Paralelismo |
|---|---|---|
| `secuencial` | Línea base (1 hilo) | — |
| `paralelo_ia` | Generado por IA | OpenMP (paraleliza por filas) |
| `paralelo_ia_opt` | IA + optimizado a mano | OpenMP + optimizaciones |
| `paralelo_manual` | Código humano (repositorio de GitHub) | OpenMP |

Todos comparten el **mismo kernel**, el mismo manejo de bordes, el mismo formato
de salida y el mismo **checksum**, de modo que la comparación es directa.

## 3. Entorno de experimentación

- **Ejecución:** contenedor Docker (Ubuntu 24.04), compilación con `g++`/CMake,
  `-O2`, OpenMP, `-ffp-contract=off` (para preservar la equivalencia bit a bit).
- **Hardware:** _(completar: CPU, número de núcleos físicos/lógicos, RAM)_.
- **Núcleos visibles por el contenedor:** _(completar con `docker run --rm ubuntu:24.04 nproc`)_.

> Para completar estos datos, ejecutar (en `cmd`):
> ```cmd
> REM Núcleos visibles por el contenedor (los que usará el benchmark)
> docker run --rm ubuntu:24.04 nproc
> REM Información del CPU y memoria de la máquina
> wmic cpu get Name,NumberOfCores,NumberOfLogicalProcessors
> wmic ComputerSystem get TotalPhysicalMemory
> REM Versión de Docker (su salida incluye CPUs y memoria vistas por Docker)
> docker info
> ```
> Mantén fijo el entorno entre corridas para que los tiempos sean comparables.

## 4. Datos de entrada

| Conjunto | Imágenes | Tamaño | Propósito |
|---|---|---|---|
| **`Images/`** | `Avion512.png`, `Male1024.png`, `nube2048.png`, `minecraft4096.png` | 512², 1024², 2048², 4096² | Evaluar la **escalabilidad por tamaño de imagen** (requisito del enunciado) |

- Se corre **una sola vez** sobre este conjunto; toda la información queda en un
  único archivo (`resultados/resultados.csv`).
- Todas las imágenes se normalizan a **RGB (3 canales)** antes de convolucionar.

## 5. Parámetros del experimento

| Parámetro | Valores |
|---|---|
| Tamaño de kernel `K` | 3, 5, 7, 11 (gaussiano normalizado) |
| Sigma | `σ = 0.3·((K−1)/2 − 1) + 0.8` |
| Borde | Replicado (*clamp*); salida del mismo tamaño que la entrada |
| Hilos | 1, 2, 4, 8, 16 |
| Repeticiones cronometradas | 30 |
| Calentamiento | 1 ejecución no cronometrada |

## 6. Métricas

Para cada configuración (imagen, kernel, hilos) se registra el **tiempo de la
convolución** en milisegundos y se calculan:

- **Media** `μ` y **desviación estándar muestral** `s` (n−1) de las 30 mediciones.
- **Mínimo** y **máximo**.
- **Aceleración (speedup):** `S(p) = T_secuencial / T_p` (misma imagen y kernel).
- **Eficiencia:** `E(p) = S(p) / p`, donde `p` es el número de hilos.

Solo se cronometra el **bucle de convolución**; la carga y (si aplica) el
guardado de la imagen quedan fuera de la medición.

## 7. Validación de equivalencia estricta

La validez de la comparación exige que todas las variantes produzcan **el mismo
resultado numérico**. Para ello:

1. Cada programa calcula un **checksum FNV-1a de 64 bits** sobre el buffer RGB de
   salida.
2. Se comprueba que el checksum de cada variante paralela sea **idéntico** al de
   la versión secuencial, para cada imagen y cada kernel, y **constante** entre
   los distintos números de hilos.
3. `docker/evaluar.py` reporta automáticamente el resumen de coincidencias
   (`X/Y coinciden`) en `resultados/tablas.md`.

Si alguna configuración no coincide, la comparación de tiempos para esa
configuración **no** es válida y se marca explícitamente en las tablas.

## 8. Protocolo (procedimiento)

> Guía detallada con solución de problemas: **`README.md`** (secciones 7 a 13).

1. **Preparar el entorno.** Abrir Docker Desktop y situarse en la carpeta del
   proyecto:
   ```cmd
   cd /d "C:\Users\JoseB\Desktop\Paralela\Benchmarking"
   docker info
   ```
2. **Construir la imagen** (compila todas las variantes presentes):
   ```cmd
   docker build -t benchmark-paralelo .
   ```
3. **Ejecutar el benchmark completo** (una sola corrida) y generar la evaluación:
   ```cmd
   docker run --rm -v "%cd%\resultados:/resultados" benchmark-paralelo
   ```
   El contenedor, para cada programa:
   - recorre automáticamente las imágenes del conjunto `Images/`;
   - aplica los kernels 3/5/7/11 y, en las variantes paralelas, prueba 1/2/4/8/16
     hilos;
   - realiza 1 ejecución de calentamiento + 30 ejecuciones cronometradas por
     configuración, y calcula las estadísticas y el checksum;
   - acumula todas las filas en **un único** `resultados/resultados.csv`.
4. **Generar tablas y figuras.** Lo hace automáticamente el paso 3 mediante
   `docker/evaluar.py`, que agrega todo (`combinado.csv`), valida los checksums y
   produce `tablas.md` y `graficas/*.png`.
   (Se puede regenerar por separado con:
   `docker run --rm -v "%cd%\resultados:/resultados" --entrypoint python3 benchmark-paralelo /app/evaluar.py /resultados`.)

Para pruebas rápidas (sin esperar los 30 ciclos completos), añadir
`-e BENCH_REPS=3` al comando del paso 3.

## 9. Resultados

- **Datos crudos:** `resultados/resultados.csv` (todas las variantes en un solo archivo).
- **Datos procesados:** `resultados/combinado.csv` (incluye `speedup` y
  `eficiencia`).
- **Tablas:** `resultados/tablas.md`.
- **Figuras:** `resultados/graficas/` (tiempo, speedup y eficiencia vs hilos por
  programa; comparación directa entre programas).

> Pegar aquí las tablas y figuras relevantes de `tablas.md` / `graficas/`.

## 10. Guía de análisis

- **Escalabilidad por tamaño:** el speedup debe crecer con el tamaño de la
  imagen porque el cómputo por píxel domina sobre los costos fijos.
- **Escalabilidad por kernel:** a mayor `K`, más trabajo por píxel y mejor
  aprovechamiento de los hilos.
- **Eficiencia:** suele decrecer al aumentar los hilos por el *overhead* de
  sincronización y la contención de memoria (ley de Amdahl). Reportar el punto
  donde la eficiencia cae por debajo de, por ejemplo, 0.5.
- **Comparación entre variantes:** contrastar el speedup de la versión
  **optimizada** frente a la versión **IA base** y frente a la **manual**,
  destacando el efecto de las optimizaciones (localidad de memoria, planificación
  de hilos, etc.).
