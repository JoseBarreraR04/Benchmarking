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

# Ruta del repositorio manual (por si su codigo necesita CImg.h).
set(MANUAL_REPO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ParaleloManualGithub/Convolution-master")

find_package(OpenMP REQUIRED)

# ----------------------------------------------------------------------------
#  Configuracion comun de los programas PARALELOS (OpenMP).
#  Nota: se mantiene -ffp-contract=off para garantizar resultados BIT A BIT
#  identicos a la version secuencial (sin contraer a FMA), de modo que los
#  checksums coincidan y la validacion estricta sea valida.
# ----------------------------------------------------------------------------
function(configurar_programa_paralelo target)
    target_include_directories(${target} PRIVATE "${THIRD_PARTY_DIR}")
    target_link_libraries(${target} PRIVATE OpenMP::OpenMP_CXX)
    # Con MinGW/GCC el flag de OpenMP tambien debe estar presente en el enlazado
    # (para enlazar libgomp); se agrega explicitamente por robustez.
    target_compile_options(${target} PRIVATE ${OpenMP_CXX_FLAGS} -ffp-contract=off)
    target_link_options(${target} PRIVATE ${OpenMP_CXX_FLAGS})
endfunction()

# --- Version secuencial (linea base, 1 hilo) ---
add_executable(secuencial secuencial.cpp)
target_include_directories(secuencial PRIVATE "${THIRD_PARTY_DIR}")
target_compile_options(secuencial PRIVATE -ffp-contract=off)

# --- Version paralela generada por IA (OpenMP) ---
add_executable(paralelo_ia paralelo_ia.cpp)
configurar_programa_paralelo(paralelo_ia)

# --- Version paralela IA + optimizada (se agrega cuando exista el archivo) ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/paralelo_ia_opt.cpp")
    add_executable(paralelo_ia_opt paralelo_ia_opt.cpp)
    configurar_programa_paralelo(paralelo_ia_opt)
endif()

# --- Version paralela MANUAL (codigo humano; se agrega cuando exista el archivo) ---
# Se soporta tanto el uso de las cabeceras stb (recomendado) como el de CImg.h
# del repositorio de origen, para que el binario compile sin cambios de librerias.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/paralelo_manual.cpp")
    add_executable(paralelo_manual paralelo_manual.cpp)
    target_include_directories(paralelo_manual PRIVATE "${THIRD_PARTY_DIR}")
    if(EXISTS "${MANUAL_REPO_DIR}/CImg.h")
        target_include_directories(paralelo_manual PRIVATE "${MANUAL_REPO_DIR}")
        # Sin ventana grafica (evita depender de X11 en Docker).
        target_compile_definitions(paralelo_manual PRIVATE cimg_display=0)
        find_package(JPEG QUIET)
        if(JPEG_FOUND)
            target_compile_definitions(paralelo_manual PRIVATE cimg_use_jpeg)
            target_link_libraries(paralelo_manual PRIVATE JPEG::JPEG)
        endif()
        find_package(PNG QUIET)
        if(PNG_FOUND)
            target_compile_definitions(paralelo_manual PRIVATE cimg_use_png)
            target_link_libraries(paralelo_manual PRIVATE PNG::PNG)
        endif()
    endif()
    target_link_libraries(paralelo_manual PRIVATE OpenMP::OpenMP_CXX)
    target_compile_options(paralelo_manual PRIVATE ${OpenMP_CXX_FLAGS} -ffp-contract=off)
    target_link_options(paralelo_manual PRIVATE ${OpenMP_CXX_FLAGS})
endif()

# En MinGW, enlazado estatico para que los .exe no dependan de DLLs externas
# (libstdc++/libgcc/libgomp). No afecta la medicion del benchmark.
# (No se aplica a paralelo_manual para no forzar enlazado estatico de libjpeg/png.)
if(MINGW)
    target_link_options(secuencial PRIVATE -static)
    target_link_options(paralelo_ia PRIVATE -static)
    if(TARGET paralelo_ia_opt)
        target_link_options(paralelo_ia_opt PRIVATE -static)
    endif()
endif()
