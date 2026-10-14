# Ejecutar el benchmark en Docker

Esta guía permite compilar y ejecutar **`secuencial`** y **`paralelo_ia`** dentro de
Docker, sin necesidad de tener C++ instalado en tu PC.

## Requisitos
- **Docker Desktop** instalado y en ejecución (con contenedores Linux / WSL2).

## Opción A (recomendada): Docker Compose

Desde la carpeta del proyecto (`Benchmarking`), en una terminal:

```powershell
docker compose up --build
```

- Construye la imagen e inicia el benchmark.
- Verás el progreso en la consola.
- Al terminar, los resultados quedan en la carpeta **`resultados/`** del proyecto:
  - `resultados/secuencial.csv`
  - `resultados/paralelo_ia.csv`

Para detenerlo antes de terminar: `Ctrl + C`.

## Opción B: Docker "a mano"

```powershell
# 1) Construir la imagen
docker build -t benchmark-paralelo .

# 2) Ejecutar montando la carpeta de resultados
docker run --rm -v "${PWD}/resultados:/resultados" benchmark-paralelo
```

## Qué hace el contenedor
1. Localiza automáticamente las imágenes en `/app/Images`.
2. Ejecuta la versión secuencial y guarda `secuencial.csv`.
3. Ejecuta la versión paralela (OpenMP, 1/2/4/8/16 hilos) y guarda `paralelo_ia.csv`.

El contenedor usa **todos los núcleos disponibles** (por defecto, todos los de tu PC).

## Formato de los CSV
```
programa,imagen,ancho,alto,kernel,hilos,repeticiones,media_ms,stddev_ms,min_ms,max_ms,checksum
```
- La columna `checksum` (FNV-1a 64 bits) debe ser **idéntica** entre `secuencial`
  y `paralelo_ia` para la misma imagen y el mismo kernel: eso valida la
  equivalencia estricta de resultados.

## Notas importantes
- **Duración:** una corrida completa es larga (el caso `4096x4096` con kernel 11x11
  domina). Para una prueba rápida, edita `REPETICIONES` (por ejemplo a `1` o `3`)
  al inicio de `secuencial.cpp` y `paralelo_ia.cpp`, y vuelve a construir con
  `docker compose up --build`.
- **Rendimiento:** los tiempos medidos dentro de Docker/WSL2 pueden diferir de los
  de la máquina "física". Si el paper exige comparación en hardware real, corre
  también fuera de Docker o reporta el entorno.
- **Núcleos del contenedor:** por defecto usa todos. Puedes limitarlos, por ejemplo,
  a 4 núcleos con `docker compose run --rm --cpus=4 benchmark` (o `docker run --cpus=4 ...`).

## Estructura relevante
```
Dockerfile            # compila (etapa builder) y ejecuta (etapa runtime)
docker-compose.yml    # ejecución con un solo comando
docker/run.sh         # script de entrada: corre ambos programas y guarda CSV
.dockerignore         # excluye build/IDE del contexto
resultados/           # (se crea al ejecutar) aquí aparecen los CSV
```
