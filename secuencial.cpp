// ============================================================================
//  secuencial.cpp
//  Version SECUENCIAL (linea base, 1 hilo) de la convolucion de imagenes.
//
//  Tarea: Benchmarking de codigo C++ paralelo generado mediante IA.
//  - Recorre AUTOMATICAMENTE todas las imagenes de la carpeta "Images".
//  - Aplica un kernel gaussiano parametrico de 3x3, 5x5, 7x7 y 11x11.
//  - Mide EXCLUSIVAMENTE el tiempo de la convolucion (30 repeticiones).
//  - Imprime una linea CSV por configuracion con tiempo, desviacion estandar
//    y un checksum (FNV-1a 64) que permite validar la equivalencia estricta
//    contra la version paralela.
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

// ============================ CONFIGURACION =================================
// Tamano de las matrices (kernel) K x K a evaluar.
static const std::vector<int> TAMANOS_KERNEL = {3, 5, 7, 11};

// Numero de ejecuciones CRONOMETRADAS por configuracion (protocolo: 30).
static const int REPETICIONES = 30;

// Ejecuciones de calentamiento NO cronometradas (estabilizan caches/CPU).
static const int CALENTAMIENTO = 1;

// Guardar la imagen resultante en disco (NO afecta la medicion).
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

// Busca una carpeta de imagenes por NOMBRE de forma robusta (distintos
// directorios de trabajo).
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

// Resuelve la carpeta de imagenes a usar:
//  - Si BENCH_DATASET apunta a un directorio, se usa tal cual (ruta absoluta
//    o relativa), lo que permite correr distintos conjuntos de imagenes.
//  - Si BENCH_DATASET es solo un nombre, se busca por ese nombre.
//  - Si no esta definida, se usa la carpeta "Images" (comportamiento original).
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

// Lee un entero desde una variable de entorno (util para acortar corridas de
// prueba sin recompilar). Devuelve 'defecto' si no esta definida o es invalida.
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
// Sigma parametrico (formula estandar tipo OpenCV) que crece con el tamano.
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

// ------------------------------ Convolucion --------------------------------
// Convolucion naif por canal con borde replicado (clamp).
// 'out' debe venir pre-dimensionado (out.size() == in.size()) para que el
// cronometraje no incluya reservas de memoria.
static void convolucionar(const std::vector<uint8_t>& in,
                          std::vector<uint8_t>& out,
                          int w, int h, int canales,
                          const std::vector<float>& kernel, int k) {
    const int r = k / 2;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            for (int c = 0; c < canales; ++c) {
                float acc = 0.0f;
                for (int ky = -r; ky <= r; ++ky) {
                    const int yy = std::clamp(y + ky, 0, h - 1);
                    const uint8_t* fila = &in[(static_cast<size_t>(yy) * w) * canales];
                    for (int kx = -r; kx <= r; ++kx) {
                        const int xx = std::clamp(x + kx, 0, w - 1);
                        acc += kernel[static_cast<size_t>(ky + r) * k + (kx + r)] *
                               fila[static_cast<size_t>(xx) * canales + c];
                    }
                }
                const int val = static_cast<int>(std::lround(acc));
                out[(static_cast<size_t>(y) * w + x) * canales + c] =
                    static_cast<uint8_t>(std::clamp(val, 0, 255));
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

    std::cerr << "[secuencial] Carpeta de imagenes: " << carpeta.string() << "\n";
    std::cerr << "[secuencial] Imagenes encontradas: " << imagenes.size()
              << " | Repeticiones: " << repeticiones << "\n";

    // Cabecera CSV (stdout) para pegar directamente en las tablas del paper.
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

        std::cerr << "\n[secuencial] Imagen " << nombre << " (" << w << "x" << h << ")\n";

        for (int k : TAMANOS_KERNEL) {
            const std::vector<float> kernel = generarKernelGaussiano(k);

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

            std::cout << "secuencial," << nombre << ',' << w << ',' << h << ',' << k
                      << ",1," << repeticiones << ','
                      << st.media << ',' << st.desviacion << ','
                      << st.minimo << ',' << st.maximo << ','
                      << chk << '\n';
            std::cout.flush();

            std::cerr << "  kernel " << k << "x" << k
                      << " -> media " << st.media << " ms | stddev " << st.desviacion
                      << " ms | checksum " << chk << "\n";

            if (GUARDAR_SALIDA) {
                const fs::path dirSalida = carpeta.parent_path() / "resultados";
                std::error_code ec;
                fs::create_directories(dirSalida, ec);
                const fs::path outImg =
                    dirSalida / (ruta.stem().string() + "_seq_k" + std::to_string(k) + ".png");
                stbi_write_png(outImg.string().c_str(), w, h, 3, salida.data(), w * 3);
            }
        }
    }

    std::cout.flush();
    return 0;
}
