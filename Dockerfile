# ============================================================================
#  Dockerfile - Entorno de compilacion y ejecucion del benchmark de convolucion
#  - Etapa 1 (builder): compila secuencial y paralelo_ia con CMake/OpenMP.
#  - Etapa 2 (runtime): imagen ligera con los ejecutables y la carpeta Images.
# ============================================================================

# ---------------------- Etapa 1: compilacion -------------------------------
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY CMakeLists.txt ./
COPY secuencial.cpp paralelo_ia.cpp ./
COPY third_party ./third_party

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j"$(nproc)"

# ---------------------- Etapa 2: ejecucion ---------------------------------
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        libgomp1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/secuencial   ./secuencial
COPY --from=builder /app/build/paralelo_ia  ./paralelo_ia
COPY Images ./Images
COPY docker/run.sh /app/run.sh
RUN sed -i 's/\r$//' /app/run.sh && chmod +x /app/run.sh

# Carpeta donde se dejan los CSV de resultados (se monta desde el host).
VOLUME ["/resultados"]

ENTRYPOINT ["/app/run.sh"]
