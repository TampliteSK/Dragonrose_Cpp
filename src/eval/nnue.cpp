// nnue.cpp -- ver nnue.hpp para la descripcion de la arquitectura.
//
// FORMATO DEL ARCHIVO (quantised.bin de bullet), verificado byte a byte
// contra la implementacion de referencia de Mittens:
//   4 bloques consecutivos de int16 little-endian, en este orden
//     1) l0w: 768*H valores, dispuestos POR FEATURE (feature 0 -> sus H
//        pesos, feature 1 -> sus H pesos, ...).
//     2) l0b: H valores.
//     3) l1w: B bloques de 2*H (primero los H "stm", luego los H "ntm").
//     4) l1b: B valores.
//   Despues del ultimo bloque el archivo lleva RELLENO hasta alinear a 64
//   bytes, y ese relleno es la palabra ASCII "bullet" repetida. Se valida:
//   si no cuadra, el archivo no es lo que creemos y se rechaza en vez de
//   cargar pesos desalineados.

#include "nnue.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "../datatypes.hpp"

namespace nnue {

namespace {

constexpr size_t N_ENTRADA = 768;
constexpr size_t H_MAX = 512;
constexpr int QA = 255;
constexpr int QB = 64;
constexpr int SCALE = 400;
constexpr size_t ALINEACION = 64;

// Arquitecturas admitidas: (capa oculta, output buckets). Cada combinacion
// da un tamano de archivo distinto, asi que el tamano identifica el formato
// sin necesidad de cabecera.
struct Arq {
    size_t h;
    size_t b;
};
constexpr Arq ARQUITECTURAS[] = {{256, 1}, {512, 1}, {512, 8}};

constexpr size_t bytes_utiles(size_t h, size_t b) {
    return (N_ENTRADA * h + h + b * 2 * h + b) * 2;
}

struct Red {
    size_t h = 0;
    size_t buckets = 0;
    std::vector<int16_t> l0w;  // [feature][neurona], 768 bloques de h
    std::vector<int16_t> l0b;  // h
    std::vector<int16_t> l1w;  // buckets bloques de 2*H_MAX (ntm empieza en H_MAX)
    std::vector<int16_t> l1b;  // buckets
};

Red g_red;
bool g_loaded = false;
bool g_enabled = false;
std::string g_desc = "sin red";

// --- Estado del acumulador incremental ---
// [0] = perspectiva blancas, [1] = perspectiva negras. Solo las primeras
// g_red.h posiciones de cada fila son significativas.
int16_t g_acc[2][H_MAX];
int g_piezas = 0;
bool g_acc_listo = false;

// Indice de feature en Chess768 visto desde `persp` (0=blancas, 1=negras).
// Convencion identica a bullet_lib::game::inputs::Chess768: las piezas
// PROPIAS van al bloque 0..384 y las del rival al 384..768; ademas, desde
// las negras la casilla se refleja verticalmente (sq ^ 56) para que la red
// vea siempre "su" primera fila abajo.
//
// OJO: `sq` aqui va en la convencion de la RED (a1=0, h8=63), que es la
// contraria a la de Dragonrose (a8=0, h1=63). La conversion se hace en
// refrescar(), no aqui.
inline size_t feature(size_t persp, size_t color, size_t pieza, size_t sq) {
    size_t propia = (color == persp) ? 0 : 384;
    size_t casilla = (persp == 0) ? sq : (sq ^ 56);
    return propia + pieza * 64 + casilla;
}

inline void sumar(int16_t* acc, size_t feat) {
    const int16_t* col = &g_red.l0w[feat * g_red.h];
    for (size_t j = 0; j < g_red.h; ++j) {
        // Suma envolvente deliberada, igual que la referencia.
        acc[j] = static_cast<int16_t>(static_cast<uint16_t>(acc[j]) + static_cast<uint16_t>(col[j]));
    }
}

// Inversa exacta de sumar(): la aritmetica envolvente sobre i16 hace que
// sumar seguido de restar (o viceversa) sea la identidad incluso si hubo
// desbordamiento intermedio.
inline void restar(int16_t* acc, size_t feat) {
    const int16_t* col = &g_red.l0w[feat * g_red.h];
    for (size_t j = 0; j < g_red.h; ++j) {
        acc[j] = static_cast<int16_t>(static_cast<uint16_t>(acc[j]) - static_cast<uint16_t>(col[j]));
    }
}

// Traduce una pieza en codificacion Dragonrose (wP..bK) a (color, tipo)
// para indexar features. color: 0=blancas, 1=negras. tipo: 0=P..5=K.
inline void color_y_tipo(int pce, size_t& color, size_t& tipo) {
    color = (pce <= wK) ? 0u : 1u;
    tipo = static_cast<size_t>((pce <= wK) ? pce - wP : pce - bP);
}

// Aplica a g_acc el efecto de anadir/quitar una pieza, en las dos
// perspectivas a la vez. `sq` en la convencion de casillas de Dragonrose
// (a8=0..h1=63); se convierte a la convencion de la red (a1=0..h8=63)
// igual que en refrescar().
template <void (*Op)(int16_t*, size_t)>
inline void actualizar_pieza(int pce, int sq_dragonrose) {
    size_t color, tipo;
    color_y_tipo(pce, color, tipo);
    size_t sq = static_cast<size_t>(sq_dragonrose ^ 56);
    Op(g_acc[0], feature(0, color, tipo, sq));
    Op(g_acc[1], feature(1, color, tipo, sq));
}

// Reconstruye los dos acumuladores desde cero y devuelve el numero total
// de piezas (para elegir el output bucket).
int refrescar(const Board& pos, int16_t acc[2][H_MAX]) {
    for (size_t p = 0; p < 2; ++p) {
        std::memcpy(acc[p], g_red.l0b.data(), g_red.h * sizeof(int16_t));
    }

    int piezas = 0;
    for (int pce = wP; pce <= bK; ++pce) {
        size_t color = (pce <= wK) ? 0u : 1u;
        size_t tipo = static_cast<size_t>((pce <= wK) ? pce - wP : pce - bP);  // 0=P .. 5=K
        Bitboard bb = pos.bitboards[pce];
        while (bb) {
            int dr_sq = __builtin_ctzll(bb);
            bb &= bb - 1;
            ++piezas;
            // Dragonrose usa a8=0..h1=63; la red usa a1=0..h8=63.
            size_t sq = static_cast<size_t>(dr_sq ^ 56);
            sumar(acc[0], feature(0, color, tipo, sq));
            sumar(acc[1], feature(1, color, tipo, sq));
        }
    }
    return piezas;
}

// Indice del output bucket segun el material. Misma formula que
// MaterialCount<N> de bullet: (piezas_totales - 2) / ceil(32/N), contando
// TODAS las piezas, reyes incluidos.
inline size_t bucket_de(int piezas) {
    if (g_red.buckets == 1) return 0;
    size_t divisor = (32 + g_red.buckets - 1) / g_red.buckets;
    size_t idx = (static_cast<size_t>(std::max(piezas, 2)) - 2) / divisor;
    return std::min(idx, g_red.buckets - 1);
}

}  // namespace

bool load(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        std::cout << "info string NNUE: no se pudo abrir " << path << std::endl;
        return false;
    }
    const std::streamsize n = f.tellg();
    f.seekg(0, std::ios::beg);
    if (n <= 0) {
        std::cout << "info string NNUE: archivo vacio" << std::endl;
        return false;
    }

    const size_t tam = static_cast<size_t>(n);
    const Arq* arq = nullptr;
    for (const Arq& a : ARQUITECTURAS) {
        size_t utiles = bytes_utiles(a.h, a.b);
        if (tam >= utiles && tam - utiles < ALINEACION) {
            arq = &a;
            break;
        }
    }
    if (arq == nullptr) {
        std::cout << "info string NNUE: tamano " << tam
                  << " no corresponde a ninguna arquitectura conocida" << std::endl;
        return false;
    }

    std::vector<uint8_t> datos(tam);
    if (!f.read(reinterpret_cast<char*>(datos.data()), n)) {
        std::cout << "info string NNUE: error leyendo el archivo" << std::endl;
        return false;
    }

    const size_t utiles = bytes_utiles(arq->h, arq->b);
    // El relleno debe ser la firma "bullet" repetida.
    static const char FIRMA[6] = {'b', 'u', 'l', 'l', 'e', 't'};
    for (size_t i = utiles; i < tam; ++i) {
        if (datos[i] != static_cast<uint8_t>(FIRMA[(i - utiles) % 6])) {
            std::cout << "info string NNUE: relleno final inesperado, se rechaza el archivo"
                      << std::endl;
            return false;
        }
    }

    const size_t h = arq->h;
    const size_t b = arq->b;
    size_t cursor = 0;
    auto leer = [&](size_t cuantos) {
        std::vector<int16_t> v(cuantos);
        for (size_t k = 0; k < cuantos; ++k) {
            v[k] = static_cast<int16_t>(static_cast<uint16_t>(datos[cursor + k * 2]) |
                                        (static_cast<uint16_t>(datos[cursor + k * 2 + 1]) << 8));
        }
        cursor += cuantos * 2;
        return v;
    };

    Red nueva;
    nueva.h = h;
    nueva.buckets = b;
    nueva.l0w = leer(N_ENTRADA * h);
    nueva.l0b = leer(h);
    nueva.l1w.assign(b * 2 * H_MAX, 0);
    for (size_t bk = 0; bk < b; ++bk) {
        std::vector<int16_t> v = leer(2 * h);
        size_t base = bk * 2 * H_MAX;
        std::copy(v.begin(), v.begin() + static_cast<long>(h), nueva.l1w.begin() + static_cast<long>(base));
        std::copy(v.begin() + static_cast<long>(h), v.end(),
                  nueva.l1w.begin() + static_cast<long>(base + H_MAX));
    }
    nueva.l1b = leer(b);

    g_red = std::move(nueva);
    g_loaded = true;
    g_desc = std::to_string(h) + " neuronas, " + std::to_string(b) + " output bucket(s)";
    // Los pesos cambiaron: el acumulador que hubiera de una red anterior ya
    // no vale nada. Se marca invalido; refresh() lo reconstruye (el llamador
    // en UciHandler.cpp hace ese refresh justo despues de un load() exitoso).
    g_acc_listo = false;
    std::cout << "info string NNUE: cargada red de " << g_desc << std::endl;
    return true;
}

bool is_loaded() { return g_loaded; }

void set_enabled(bool on) { g_enabled = on; }

bool is_enabled() { return g_enabled && g_loaded; }

std::string description() { return g_desc; }

void refresh(const Board& pos) {
    if (!g_loaded) return;
    g_piezas = refrescar(pos, g_acc);
    g_acc_listo = true;
}

void on_add_piece(int pce, int sq) {
    if (!g_loaded) return;
    actualizar_pieza<sumar>(pce, sq);
    ++g_piezas;
}

void on_remove_piece(int pce, int sq) {
    if (!g_loaded) return;
    actualizar_pieza<restar>(pce, sq);
    --g_piezas;
}

bool consistente_con_recalculo(const Board& pos) {
    if (!g_loaded) return true;
    int16_t tmp[2][H_MAX];
    const int piezas = refrescar(pos, tmp);
    if (piezas != g_piezas) return false;
    return std::memcmp(g_acc[0], tmp[0], g_red.h * sizeof(int16_t)) == 0 &&
           std::memcmp(g_acc[1], tmp[1], g_red.h * sizeof(int16_t)) == 0;
}

int evaluate(const Board& pos) {
    // El acumulador se mantiene incrementalmente (refresh() en parse_fen +
    // ganchos on_add_piece/on_remove_piece en make/unmake). Si por lo que
    // sea nunca se inicializo para esta posicion (p.ej. UseNNUE se activo
    // sin que hubiera una red cargada al hacer el ultimo parse_fen), se
    // reconstruye aqui como red de seguridad -- no deberia pasar en uso
    // normal, pero evita evaluar con basura sin inicializar.
    if (!g_acc_listo) {
        refresh(pos);
    }

    const bool negras_mueven = (pos.side == BLACK);
    const int16_t* yo = negras_mueven ? g_acc[1] : g_acc[0];
    const int16_t* rival = negras_mueven ? g_acc[0] : g_acc[1];

    const size_t bk = bucket_de(g_piezas);
    const int16_t* w = &g_red.l1w[bk * 2 * H_MAX];

    // SCReLU: clamp(x, 0, QA)^2. El cuadrado deja la escala en QA^2*QB, por
    // eso despues se divide UNA vez entre QA para volver a QA*QB, que es la
    // escala en la que bullet guardo l1b.
    int64_t suma = 0;
    for (size_t j = 0; j < g_red.h; ++j) {
        int64_t v = std::clamp<int64_t>(yo[j], 0, QA);
        suma += v * v * w[j];
        int64_t u = std::clamp<int64_t>(rival[j], 0, QA);
        suma += u * u * w[H_MAX + j];
    }

    const int64_t salida = suma / QA + g_red.l1b[bk];
    return static_cast<int>(std::lround(static_cast<double>(salida * SCALE) / (QA * QB)));
}

}  // namespace nnue
