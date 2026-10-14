// ============================================================================
//  paralelo_ia_opt.cpp
//  Version PARALELA generada por IA y OPTIMIZADA de la convolucion de imagenes.
//
//  Parte de 'paralelo_ia' y aplica DOS optimizaciones que suelen pasarse por
//  alto, SIN cambiar el resultado (checksum identico bit a bit a la secuencial):
//
//   1) INVERSION DE BUCLES + acumulador por pixel.
//      En 'paralelo_ia' el bucle mas interno es el del kernel (3..11 vueltas)
//      y todas las sumas van a UNA variable, lo que impide vectorizar. Aqui el
//      bucle mas interno recorre toda la FILA (pixeles contiguos en memoria) y
//      cada pixel tiene su propio acumulador: eso es vectorizable (SIMD).
//
//          para cada fila y (repartida entre hilos)
//            acum[] = 0                        (uno por pixel de la fila)
//            para cada ky del kernel
//              para cada kx del kernel
//                peso = kernel[ky][kx]
//                para cada x                  <- bucle largo, vectorizable
//                  acum[x] += peso * pixel(y+ky, x+kx)
//            escribir acum[x] limitado a 0..255
//
//   2) FUSION DE CANALES RGB.
//      La secuencial lee el vecindario del kernel 3 VECES (una por canal). Aqui
//      se leen los 3 canales de un pixel de una sola vez y se acumulan en
//      acumR/acumG/acumB, reduciendo los accesos a memoria.
//
//  CLAVE de la equivalencia estricta: para cada pixel y canal, las sumas se
//  hacen SIEMPRE en el mismo orden (primero 'ky', luego 'kx') que la secuencial,
//  en 'float' y sin FMA (el CMake compila con -ffp-contract=off). Por eso el
//  checksum FNV-1a coincide EXACTAMENTE con el de 'secuencial'.
//
//  Uso:  ejecutar directamente (no requiere argumentos).
// ============================================================================

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

// ============================ CONFIGURACION =================================
static const std::vector<int> TAMANOS_KERNEL = {3, 5, 7, 11};
static const std::vector<int> NUM_HILOS = {1, 2, 4, 8, 16};
static const int REPETICIONES = 30;
static const int CALENTAMIENTO = 1;
static const bool GUARDAR_SALIDA = false;
// ============================================================================

namespace fs = std::filesystem;

// ------------------------- Utilidades de imagenes --------------------------
static bool esImagen(const fs::path& p) {
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
           ext == ".bmp" || ext == ".tga";
}

static fs::path dirEjecutable(const char* argv0) {
    std::error_code ec;
    fs::path p = fs::absolute(argv0 ? argv0 : ".", ec);
    if (ec) return {};
    return p.parent_path();
}

static fs::path encontrarCarpetaImagenes(const fs::path& exeDir, const std::string& nombre) {
    std::vector<fs::path> candidatos;
    std::error_code ec;
    fs::path cwd = fs::current_path(ec);
    candidatos.push_back(cwd / nombre);
    candidatos.push_back(cwd / ".." / nombre);
    candidatos.push_back(cwd / ".." / ".." / nombre);
    candidatos.push_back(cwd / ".." / ".." / ".." / nombre);
    if (!exeDir.empty()) {
        candidatos.push_back(exeDir / nombre);
        candidatos.push_back(exeDir / ".." / nombre);
        candidatos.push_back(exeDir / ".." / ".." / nombre);
    }
    for (const auto& c : candidatos) {
        std::error_code e;
        if (!fs::is_directory(c, e)) continue;
        for (const auto& entry : fs::directory_iterator(c, e)) {
            if (entry.is_regular_file() && esImagen(entry.path())) {
                return fs::canonical(c, e);
            }
        }
    }
    return {};
}

static fs::path resolverCarpeta(const fs::path& exeDir) {
    const char* env = std::getenv("BENCH_DATASET");
    if (env && *env) {
        std::error_code e;
        fs::path p(env);
        if (fs::is_directory(p, e)) return fs::canonical(p, e);
        return encontrarCarpetaImagenes(exeDir, env);
    }
    return encontrarCarpetaImagenes(exeDir, "Images");
}

static int leerEnteroEnv(const char* nombre, int defecto) {
    const char* env = std::getenv(nombre);
    if (!env || !*env) return defecto;
    try {
        const int v = std::stoi(env);
        return (v > 0) ? v : defecto;
    } catch (...) {
        return defecto;
    }
}

static std::vector<fs::path> listarImagenes(const fs::path& carpeta) {
    std::vector<fs::path> imgs;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(carpeta, ec)) {
        if (entry.is_regular_file() && esImagen(entry.path()))
            imgs.push_back(entry.path());
    }
    std::sort(imgs.begin(), imgs.end());
    return imgs;
}

// --------------------------- Kernel gaussiano ------------------------------
static std::vector<float> generarKernelGaussiano(int k) {
    const float sigma = 0.3f * ((k - 1) * 0.5f - 1.0f) + 0.8f;
    const int r = k / 2;
    std::vector<float> kernel(static_cast<size_t>(k) * k);
    float suma = 0.0f;
    for (int y = -r; y <= r; ++y) {
        for (int x = -r; x <= r; ++x) {
            const float v = std::exp(-(x * x + y * y) / (2.0f * sigma * sigma));
            kernel[static_cast<size_t>(y + r) * k + (x + r)] = v;
            suma += v;
        }
    }
    for (float& v : kernel) v /= suma;  // normalizar => suma = 1
    return kernel;
}

// ========================== CONVOLUCION OPTIMIZADA =========================
// Inversion de bucles + acumulador por pixel + fusion de canales.
// Cada hilo reutiliza su propio buffer 'acum' (se reserva una vez por hilo,
// no por fila) para no penalizar con reservas de memoria dentro del bucle.
static void convolucionar(const std::vector<uint8_t>& in,
                          std::vector<uint8_t>& out,
                          int w, int h, int canales,
                          const std::vector<float>& kernel, int k) {
    const int r = k / 2;
    const size_t filaBytes = static_cast<size_t>(w) * canales;

#ifdef _OPENMP
#pragma omp parallel
#endif
    {
        std::vector<float> acum(filaBytes, 0.0f);  // un acumulador por pixel (todos los canales)

#ifdef _OPENMP
#pragma omp for schedule(static)
#endif
        for (int y = 0; y < h; ++y) {
            std::fill(acum.begin(), acum.end(), 0.0f);

            for (int ky = -r; ky <= r; ++ky) {
                const int yy = std::clamp(y + ky, 0, h - 1);
                const uint8_t* fila = &in[static_cast<size_t>(yy) * filaBytes];
                const float* kfila = &kernel[static_cast<size_t>(ky + r) * k];
                for (int kx = -r; kx <= r; ++kx) {
                    const float peso = kfila[kx + r];
                    // Bucle largo y contiguo: cada pixel tiene su acumulador.
                    for (int x = 0; x < w; ++x) {
                        const int xx = std::clamp(x + kx, 0, w - 1);
                        const uint8_t* p = &fila[static_cast<size_t>(xx) * canales];
                        float* a = &acum[static_cast<size_t>(x) * canales];
                        // Se leen los 3 canales juntos (menos accesos a memoria).
                        // Mismo orden (ky, kx) por canal => bit a bit igual.
                        for (int c = 0; c < canales; ++c)
                            a[c] += peso * static_cast<float>(p[c]);
                    }
                }
            }

            for (int x = 0; x < w; ++x) {
                const float* a = &acum[static_cast<size_t>(x) * canales];
                for (int c = 0; c < canales; ++c) {
                    const int val = static_cast<int>(std::lround(a[c]));
                    out[(static_cast<size_t>(y) * w + x) * canales + c] =
                        static_cast<uint8_t>(std::clamp(val, 0, 255));
                }
            }
        }
    }
}

// ------------------------------- Checksum ----------------------------------
static uint64_t fnv1a64(const std::vector<uint8_t>& datos) {
    uint64_t hash = 1469598103934665603ULL;
    for (uint8_t b : datos) {
        hash ^= b;
        hash *= 1099511628211ULL;
    }
    return hash;
}

// ------------------------------ Estadisticas -------------------------------
struct Estadisticas {
    double media;
    double desviacion;
    double minimo;
    double maximo;
};

static Estadisticas calcularEstadisticas(const std::vector<double>& v) {
    Estadisticas st{0.0, 0.0, 0.0, 0.0};
    if (v.empty()) return st;
    double suma = 0.0, mn = v[0], mx = v[0];
    for (double x : v) {
        suma += x;
        mn = std::min(mn, x);
        mx = std::max(mx, x);
    }
    st.media = suma / static_cast<double>(v.size());
    st.minimo = mn;
    st.maximo = mx;
    if (v.size() > 1) {
        double var = 0.0;
        for (double x : v) var += (x - st.media) * (x - st.media);
        st.desviacion = std::sqrt(var / static_cast<double>(v.size() - 1));  // muestral
    }
    return st;
}

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    const fs::path carpeta = resolverCarpeta(dirEjecutable(argc > 0 ? argv[0] : nullptr));
    if (carpeta.empty()) {
        std::cerr << "[ERROR] No se encontro una carpeta de imagenes valida. "
                     "Crea 'Images' o define la variable BENCH_DATASET.\n";
        return 1;
    }
    const std::vector<fs::path> imagenes = listarImagenes(carpeta);
    if (imagenes.empty()) {
        std::cerr << "[ERROR] La carpeta '" << carpeta.string() << "' esta vacia.\n";
        return 1;
    }

    const int repeticiones = leerEnteroEnv("BENCH_REPS", REPETICIONES);

#ifdef _OPENMP
    const int hilosMax = omp_get_max_threads();
#else
    const int hilosMax = 1;
    std::cerr << "[AVISO] Compilado SIN OpenMP: se ejecutara en 1 hilo.\n";
#endif

    std::cerr << "[paralelo_ia_opt] Carpeta de imagenes: " << carpeta.string() << "\n";
    std::cerr << "[paralelo_ia_opt] Imagenes encontradas: " << imagenes.size()
              << " | Hilos maximos: " << hilosMax
              << " | Repeticiones: " << repeticiones << "\n";

    std::cout << "programa,imagen,ancho,alto,kernel,hilos,repeticiones,"
                 "media_ms,stddev_ms,min_ms,max_ms,checksum\n";
    std::cout << std::fixed << std::setprecision(4);

    for (const fs::path& ruta : imagenes) {
        int w = 0, h = 0, canales = 0;
        unsigned char* data = stbi_load(ruta.string().c_str(), &w, &h, &canales, 3);
        if (!data) {
            std::cerr << "[ERROR] No se pudo cargar: " << ruta.string() << "\n";
            continue;
        }
        std::vector<uint8_t> entrada(data, data + static_cast<size_t>(w) * h * 3);
        stbi_image_free(data);

        std::vector<uint8_t> salida(entrada.size());
        const std::string nombre = ruta.filename().string();

        std::cerr << "\n[paralelo_ia_opt] Imagen " << nombre << " (" << w << "x" << h << ")\n";

        for (int k : TAMANOS_KERNEL) {
            const std::vector<float> kernel = generarKernelGaussiano(k);

            for (int hilos : NUM_HILOS) {
#ifdef _OPENMP
                omp_set_num_threads(hilos);
#endif
                for (int i = 0; i < CALENTAMIENTO; ++i)
                    convolucionar(entrada, salida, w, h, 3, kernel, k);

                std::vector<double> tiempos;
                tiempos.reserve(repeticiones);
                for (int rep = 0; rep < repeticiones; ++rep) {
                    const auto t0 = std::chrono::high_resolution_clock::now();
                    convolucionar(entrada, salida, w, h, 3, kernel, k);
                    const auto t1 = std::chrono::high_resolution_clock::now();
                    tiempos.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
                }

                const Estadisticas st = calcularEstadisticas(tiempos);
                const uint64_t chk = fnv1a64(salida);

                std::cout << "paralelo_ia_opt," << nombre << ',' << w << ',' << h << ',' << k
                          << ',' << hilos << ',' << repeticiones << ','
                          << st.media << ',' << st.desviacion << ','
                          << st.minimo << ',' << st.maximo << ','
                          << chk << '\n';
                std::cout.flush();

                std::cerr << "  kernel " << k << "x" << k << " | " << hilos << " hilo(s)"
                          << " -> media " << st.media << " ms | stddev " << st.desviacion
                          << " ms | checksum " << chk << "\n";

                if (GUARDAR_SALIDA) {
                    const fs::path dirSalida = carpeta.parent_path() / "resultados";
                    std::error_code ec;
                    fs::create_directories(dirSalida, ec);
                    const fs::path outImg = dirSalida /
                        (ruta.stem().string() + "_opt_k" + std::to_string(k) +
                         "_t" + std::to_string(hilos) + ".png");
                    stbi_write_png(outImg.string().c_str(), w, h, 3, salida.data(), w * 3);
                }
            }
        }
    }

    std::cout.flush();
    return 0;
}
