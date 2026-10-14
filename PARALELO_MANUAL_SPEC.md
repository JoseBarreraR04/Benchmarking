# Especificación: `paralelo_manual.cpp` (variante hecha a mano)

Este documento define **el contrato** que debe cumplir tu adaptación del código
manual (el de <https://github.com/ivandres73/Convolution.git>) para que sea
**comparable** con `secuencial` y
`paralelo_ia` bajo exactamente las mismas condiciones. El **algoritmo de
convolución sigue siendo tuyo** (humano); aquí solo se fija el formato y las
condiciones experimentales.

## Ubicación y compilación

- Coloca el archivo en la **raíz del proyecto**: `Benchmarking/paralelo_manual.cpp`.
- `CMakeLists.txt` lo detecta automáticamente (`if(EXISTS ...)`) y lo compila
  como el ejecutable `paralelo_manual`. **No** hay que tocar nada más.
- Si usas las cabeceras `stb_image.h` / `stb_image_write.h` (recomendado, ya
  están en `third_party/`), no necesitas librerías externas. Si prefieres
  conservar `CImg.h`, el CMake ya lo soporta (define `cimg_display=0` y enlaza
  `libjpeg`/`libpng`).
- Si usas OpenMP, añade `#include <omp.h>` y `#pragma omp parallel for ...`.

## Condiciones experimentales obligatorias (las mismas que las otras variantes)

| Aspecto | Valor |
|---|---|
| Imágenes | todas las que haya en la carpeta indicada por `BENCH_DATASET` (por defecto `Images`) |
| Canales | forzar **RGB (3 canales)** al cargar |
| Kernel | **gaussiano**, tamaños **3, 5, 7, 11** |
| Sigma | `sigma = 0.3*((k-1)*0.5 - 1) + 0.8`, kernel **normalizado** (suma = 1) |
| Borde | **replicado (clamp)**: la salida tiene el **mismo tamaño** que la entrada |
| Hilos | **1, 2, 4, 8, 16** |
| Repeticiones | **30** cronometradas + 1 de calentamiento **no** cronometrada |
| Medición | medir **solo** la convolución (no cargar/guardar imagen) |

> ⚠️ Tu código original usa un kernel **fijo 3×3**, tamaño **fijo 1080×720**,
> **sin borde** y acumula en `unsigned short` (se desborda). Todo eso hay que
> ajustarlo a la tabla de arriba para que la comparación sea justa.

## Salida obligatoria (a **stdout**)

Debe imprimir **una línea CSV por configuración**, con este encabezado exacto:

```
programa,imagen,ancho,alto,kernel,hilos,repeticiones,media_ms,stddev_ms,min_ms,max_ms,checksum
```

- `programa` = `paralelo_manual`
- `checksum` = **FNV-1a de 64 bits** sobre el buffer RGB de salida (uint8),
  idéntico al de las otras variantes. Con el mismo kernel e imagen, **debe
  coincidir** con el de `secuencial`.
- Los mensajes de progreso (logs) van a **stderr**, para no ensuciar el CSV.

## Ayudas que puedes reutilizar tal cual

Estos bloques son infraestructura común (idénticos en las otras variantes); no
son el "algoritmo", así que puedes copiarlos de `secuencial.cpp`:

- `esImagen`, `dirEjecutable`, `encontrarCarpetaImagenes`, `resolverCarpeta`,
  `listarImagenes`, `leerEnteroEnv`
- `generarKernelGaussiano(k)`
- `fnv1a64(...)`
- `calcularEstadisticas(...)`
- El esqueleto de `main()` (recorrer imágenes, bucles de kernel/hilos,
  calentamiento + 30 repeticiones con `std::chrono`, e impresión del CSV).

Lo único que debes aportar es **tu función de convolución** (la del repo),
adaptada a tamaño dinámico, kernel dinámico y borde clamp.

## Cómo integrarlo y correrlo (paso a paso)

1. **Crea el archivo.** Copia tu código del repositorio
   <https://github.com/ivandres73/Convolution.git> a la raíz del
   proyecto con el nombre exacto `paralelo_manual.cpp` y ajústalo a la tabla de
   condiciones de arriba.

2. **Reconstruye la imagen** (para que CMake lo detecte y compile):
   ```cmd
   cd /d "C:\Users\JoseB\Desktop\Paralela\Benchmarking"
   docker build -t benchmark-paralelo .
   ```
   Durante la compilación verás la línea `Built target paralelo_manual`. Si no
   aparece, el archivo no está en la raíz o tiene errores de compilación (revisa
   el mensaje de error).

3. **Corre el benchmark completo** (ya incluirá `paralelo_manual`):
   ```cmd
   docker run --rm -v "%cd%\resultados:/resultados" benchmark-paralelo
   ```
   > Para probar rápido antes de la corrida completa:
   > `docker run --rm -e BENCH_REPS=3 -e BENCH_DATASETS=setA -v "%cd%\resultados:/resultados" benchmark-paralelo`

4. **Verifica la equivalencia.** Abre `resultados/resumen.txt`: debe decir
   `X/X coinciden, 0 fallos`. Si tu variante manual falla el checksum, revisa en
   `resultados/tablas.md` (sección 1) qué imagen/kernel no coincide.

5. **Comprueba tu CSV** directamente si quieres ver el detalle:
   `resultados/setA/paralelo_manual.csv`.

### Correr solo el manual (a mano)

Si ya construiste la imagen y solo quieres ejecutar tu variante:

```cmd
docker run --rm -v "%cd%\resultados:/resultados" --entrypoint sh benchmark-paralelo -c "mkdir -p /resultados/setA && BENCH_DATASET=/app/Images /app/paralelo_manual > /resultados/setA/paralelo_manual.csv"
```

Y luego regenerar la evaluación con:

```cmd
docker run --rm -v "%cd%\resultados:/resultados" --entrypoint python3 benchmark-paralelo /app/evaluar.py /resultados
```

## Verificación rápida

Tras compilar dentro de Docker, la columna `checksum` de
`resultados/setA/paralelo_manual.csv` debe ser **igual** a la de
`resultados/setA/secuencial.csv` para cada imagen y kernel. El script
`evaluar.py` comprueba esto automáticamente y lo reporta en `tablas.md`.

## Si el checksum no coincide

Causas típicas (todas rompen la equivalencia bit a bit):

- Usar `float` con otro orden de suma de los taps del kernel, o usar `double`.
- Activar `-ffast-math`, `-Ofast` o FMA (`-mfma`) y dejar que el compilador
  contraiga operaciones.
- Redondear o truncar el resultado a `uint8` en un momento distinto.
- Manejo de borde distinto (clamp vs. cero vs. recortar la salida).
- Kernel generado con otra `sigma` o sin normalizar.

Regla de oro: **mismo orden de operaciones y mismo tipo (`float`) que
`secuencial.cpp`**.
