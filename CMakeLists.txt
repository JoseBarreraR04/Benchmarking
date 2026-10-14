cmake_minimum_required(VERSION 3.20)
project(Benchmarking)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compilacion optimizada por defecto para que el benchmark sea representativo.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# Cabeceras de terceros (stb_image / stb_image_write).
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# --- Version secuencial (linea base, 1 hilo) ---
add_executable(secuencial secuencial.cpp)
target_include_directories(secuencial PRIVATE "${THIRD_PARTY_DIR}")

# --- Version paralela generada por IA (OpenMP) ---
find_package(OpenMP REQUIRED)
add_executable(paralelo_ia paralelo_ia.cpp)
target_include_directories(paralelo_ia PRIVATE "${THIRD_PARTY_DIR}")
target_link_libraries(paralelo_ia PRIVATE OpenMP::OpenMP_CXX)
# Con MinGW/GCC el flag de OpenMP tambien debe estar presente en el enlazado
# (para enlazar libgomp); se agrega explicitamente por robustez entre versiones.
target_compile_options(paralelo_ia PRIVATE ${OpenMP_CXX_FLAGS})
target_link_options(paralelo_ia PRIVATE ${OpenMP_CXX_FLAGS})

# En MinGW, enlazado estatico para que los .exe no dependan de DLLs externas
# (libstdc++/libgcc/libgomp). No afecta la medicion del benchmark.
if(MINGW)
    target_link_options(secuencial PRIVATE -static)
    target_link_options(paralelo_ia PRIVATE -static)
endif()
