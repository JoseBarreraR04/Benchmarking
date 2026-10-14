# Ejecutar el benchmark en Docker

Esta guía permite compilar y ejecutar **todas las variantes** del benchmark
dentro de Docker, sin necesidad de tener C++ instalado en tu PC, y genera
automáticamente los datos y las gráficas para el paper.

## Requisitos
- **Docker Desktop** instalado y en ejecución (con contenedores Linux / WSL2).

## Opción A (recomendada): Docker Compose

Desde la carpeta del proyecto (`Benchmarking`), en una terminal:

```powershell
docker compose up --build
```

Esto:
1. Compila **todas** las variantes que existan como `.cpp` en la raíz
   (`secuencial`, `paralelo_ia`, y automáticamente `paralelo_ia_opt` y
   `paralelo_manual` si añades esos archivos).
2. Corre cada programa sobre los conjuntos de imágenes.
3. Genera la evaluación (CSV combinado, tablas Markdown y gráficas).

Al terminar, todo queda en **`resultados/`**:

```
resultados/
├── setA/                     # 512, 1024, 2048, 4096 px (enunciado de la tarea)
│   ├── secuencial.csv
│   ├── paralelo_ia.csv
│   ├── paralelo_manual.csv   # (si existe el .cpp)
│   └── ...
├── setB/                     # 30 imágenes aleatorias de imgs/ (1080x720)
│   └── ...
├── manifiesto_setB.txt       # qué 30 imágenes entraron en setB
├── combinado.csv             # todo junto + speedup + eficiencia
├── tablas.md                 # tablas Markdown listas para el paper
├── resumen.txt               # resumen de la corrida
└── graficas/                 # *.png (tiempo, speedup, eficiencia vs hilos)
```

Para detenerlo antes de terminar: `Ctrl + C`. Para limpiar el contenedor:
`docker compose down`.

## Opción B: Docker "a mano"

```powershell
# 1) Construir la imagen
docker build -t benchmark-paralelo .

# 2) Ejecutar montando la carpeta de resultados
docker run --rm -v "${PWD}/resultados:/resultados" benchmark-paralelo
```

## Variables de entorno opcionales

| Variable | Descripción | Por defecto |
|---|---|---|
| `BENCH_REPS` | Repeticiones **cronometradas** por configuración | `30` |
| `BENCH_DATASETS` | Conjuntos a correr: `setA` y/o `setB` | `setA setB` |

Ejemplos:

```powershell
# Prueba rápida (3 repeticiones, solo el conjunto de la tarea)
$env:BENCH_REPS=3; $env:BENCH_DATASETS="setA"; docker compose up --build
```

```powershell
# Todo completo con el protocolo de 30 repeticiones
docker compose up --build
```

## Qué mide cada programa

- Recorre **automáticamente** todas las imágenes de la carpeta (sin argumentos).
- Kernel **gaussiano** de 3×3, 5×5, 7×7 y 11×11.
- Hilos (**solo en las variantes paralelas**): 1, 2, 4, 8 y 16.
- **30 repeticiones** cronometradas + 1 de calentamiento no cronometrada.
- Solo se cronometra **la convolución** (no cargar/guardar la imagen).

## Formato de los CSV

```
programa,imagen,ancho,alto,kernel,hilos,repeticiones,media_ms,stddev_ms,min_ms,max_ms,checksum
```

- La columna `checksum` (FNV-1a 64 bits) debe ser **idéntica** entre `secuencial`
  y cada variante paralela para la misma imagen y kernel: eso valida la
  **equivalencia estricta** de resultados y hace justa la comparación de tiempos.
- El script `docker/evaluar.py` comprueba esto automáticamente y lo reporta en
  `resultados/tablas.md`.

## Añadir las variantes que faltan

- **Manual** (`paralelo_manual.cpp`): ver `PARALELO_MANUAL_SPEC.md`.
- **IA optimizada** (`paralelo_ia_opt.cpp`): mismo formato de CSV que
  `paralelo_ia`, manteniendo el resultado **bit a bit** igual (no usar
  `-ffast-math`, no reordenar la suma de taps, no usar FMA).

En ambos casos basta con **crear el `.cpp` en la raíz**: CMake y Docker lo
detectan solos, no hay que cambiar ninguna configuración.

## Notas importantes

- **Duración:** el conjunto `setA` es el más costoso (el caso `4096×4096` con
  kernel 11×11 domina). Para pruebas rápidas usa `BENCH_REPS=3`.
- **Rendimiento:** los tiempos dentro de Docker/WSL2 pueden diferir de la
  máquina "física". Si el paper exige hardware real, reporta el entorno en la
  sección de metodología.
- **Núcleos del contenedor:** por defecto usa todos los visibles. Puedes
  limitarlos con `docker run --cpus=4 ...`.

## Estructura relevante

```
Dockerfile            # etapa builder (compila) + etapa runtime (ejecuta + evalúa)
docker-compose.yml    # ejecución con un solo comando
docker/run_all.sh     # corre todas las variantes sobre setA y setB
docker/evaluar.py     # genera combinado.csv, tablas.md y graficas/
.dockerignore         # excluye build/IDE/resultados del contexto
resultados/           # aquí aparecen todos los resultados
```
