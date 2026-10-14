# ============================================================================
#  Dockerfile - Entorno de compilacion y ejecucion del benchmark de convolucion
#
#  Etapa 1 (builder): compila TODAS las variantes que existan como .cpp:
#                     secuencial, paralelo_ia, paralelo_ia_opt y paralelo_manual.
#                     (Las variantes opcionales se incluyen solas si el archivo
#                      existe; ver los "if(EXISTS ...)" en CMakeLists.txt.)
#  Etapa 2 (runtime): imagen ligera con los ejecutables, las imagenes y el
#                     script que corre todo y genera tablas + graficas.
# ============================================================================

# ---------------------- Etapa 1: compilacion -------------------------------
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        libjpeg-dev \
        libpng-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
# Se copia todo el contexto (docker ignora build/, resultados/, .git, etc.).
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j"$(nproc)" \
    && mkdir -p /out \
    && cp build/secuencial  /out/ \
    && cp build/paralelo_ia /out/ \
    && for p in paralelo_ia_opt paralelo_manual; do \
           if [ -f "build/$p" ]; then cp "build/$p" /out/; fi; \
       done

# ---------------------- Etapa 2: ejecucion ---------------------------------
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
# libgomp1: runtime de OpenMP. python3 + matplotlib: evaluacion (tablas y graficas).
# libjpeg/libpng: solo se necesitan si el codigo MANUAL usa CImg (se instalan de
# forma opcional; si el usuario usa las cabeceras stb no hacen falta).
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        libgomp1 \
        python3 \
        python3-matplotlib \
    && (apt-get install -y --no-install-recommends libjpeg-turbo8 libpng16-16t64 || true) \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /out/ ./
# Un solo conjunto de imagenes (512, 1024, 2048 y 4096 px).
COPY Images ./Images
COPY docker/run_all.sh /app/run_all.sh
COPY docker/evaluar.py /app/evaluar.py
RUN sed -i 's/\r$//' /app/run_all.sh /app/evaluar.py && chmod +x /app/run_all.sh

# Carpeta donde se dejan los resultados (se monta desde el host).
VOLUME ["/resultados"]

ENTRYPOINT ["/app/run_all.sh"]
