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
#include <climits>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "../datatypes.hpp"
#include "nnue_pesos_embebidos.hpp"

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
bool g_enabled = true;
std::string g_desc = "sin red";

// Punteros crudos a los pesos (copia de los de g_red). Evitan tener que
// releer el puntero interno de cada std::vector en cada acceso dentro de
// los bucles calientes, y permiten marcarlos __restrict.
const int16_t* g_l0w = nullptr;
const int16_t* g_l0b = nullptr;
const int16_t* g_l1w = nullptr;
const int16_t* g_l1b = nullptr;
size_t g_h = 0;

// Bloque de acumulacion en int32 para el producto escalar de salida (ver
// propagar()). g_rapido dice si ese camino es exacto para ESTOS pesos.
constexpr size_t BLOQUE = 32;
bool g_rapido = false;

// --- Estado del acumulador incremental ---
// [0] = perspectiva blancas, [1] = perspectiva negras. Solo las primeras
// g_red.h posiciones de cada fila son significativas.
alignas(64) int16_t g_acc[2][H_MAX];
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

// Nucleo de actualizacion del acumulador. `H` es constante de compilacion
// en las llamadas calientes (256/512), lo que permite al compilador
// desenrollar y vectorizar por completo; los __restrict eliminan el chequeo
// de solapamiento en tiempo de ejecucion que antes forzaba un camino
// escalar. La aritmetica envolvente sobre i16 es deliberada, igual que la
// referencia: sumar y restar son inversas exactas aunque haya desbordado.
template <size_t H, bool ADD>
inline void nucleo(int16_t* __restrict acc, const int16_t* __restrict col) {
    for (size_t j = 0; j < H; ++j) {
        const uint16_t a = static_cast<uint16_t>(acc[j]);
        const uint16_t c = static_cast<uint16_t>(col[j]);
        acc[j] = static_cast<int16_t>(static_cast<uint16_t>(ADD ? (a + c) : (a - c)));
    }
}

// Nucleo fusionado quitar+poner: acc += poner - quitar en una sola pasada.
// La aritmetica modulo 2^16 hace que sea EXACTAMENTE lo mismo que aplicar
// restar() y luego sumar(), pero con la mitad de trafico de memoria.
template <size_t H>
inline void nucleo_mover(int16_t* __restrict acc, const int16_t* __restrict quitar,
                         const int16_t* __restrict poner) {
    for (size_t j = 0; j < H; ++j) {
        const uint16_t a = static_cast<uint16_t>(acc[j]);
        const uint16_t q = static_cast<uint16_t>(quitar[j]);
        const uint16_t p = static_cast<uint16_t>(poner[j]);
        acc[j] = static_cast<int16_t>(static_cast<uint16_t>(a - q + p));
    }
}

inline void nucleo_mover_gen(int16_t* __restrict acc, const int16_t* __restrict quitar,
                             const int16_t* __restrict poner, size_t h) {
    for (size_t j = 0; j < h; ++j) {
        const uint16_t a = static_cast<uint16_t>(acc[j]);
        const uint16_t q = static_cast<uint16_t>(quitar[j]);
        const uint16_t p = static_cast<uint16_t>(poner[j]);
        acc[j] = static_cast<int16_t>(static_cast<uint16_t>(a - q + p));
    }
}

// Version con longitud en tiempo de ejecucion (arquitecturas no previstas
// y reconstruccion completa, que no estan en el camino caliente).
template <bool ADD>
inline void nucleo_gen(int16_t* __restrict acc, const int16_t* __restrict col, size_t h) {
    for (size_t j = 0; j < h; ++j) {
        const uint16_t a = static_cast<uint16_t>(acc[j]);
        const uint16_t c = static_cast<uint16_t>(col[j]);
        acc[j] = static_cast<int16_t>(static_cast<uint16_t>(ADD ? (a + c) : (a - c)));
    }
}

inline void sumar(int16_t* acc, size_t feat) {
    nucleo_gen<true>(acc, g_l0w + feat * g_h, g_h);
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
template <bool ADD>
inline void actualizar_pieza(int pce, int sq_dragonrose) {
    size_t color, tipo;
    color_y_tipo(pce, color, tipo);
    const size_t sq = static_cast<size_t>(sq_dragonrose ^ 56);
    const size_t f0 = feature(0, color, tipo, sq);
    const size_t f1 = feature(1, color, tipo, sq);
    // Despacho sobre H: las dos arquitecturas reales (512 y 256) usan la
    // version especializada, totalmente desenrollada/vectorizada.
    if (g_h == 512) {
        nucleo<512, ADD>(g_acc[0], g_l0w + f0 * 512);
        nucleo<512, ADD>(g_acc[1], g_l0w + f1 * 512);
    } else if (g_h == 256) {
        nucleo<256, ADD>(g_acc[0], g_l0w + f0 * 256);
        nucleo<256, ADD>(g_acc[1], g_l0w + f1 * 256);
    } else {
        nucleo_gen<ADD>(g_acc[0], g_l0w + f0 * g_h, g_h);
        nucleo_gen<ADD>(g_acc[1], g_l0w + f1 * g_h, g_h);
    }
}

// Reconstruye los dos acumuladores desde cero y devuelve el numero total
// de piezas (para elegir el output bucket).
int refrescar(const Board& pos, int16_t acc[2][H_MAX]) {
    for (size_t p = 0; p < 2; ++p) {
        std::memcpy(acc[p], g_l0b, g_h * sizeof(int16_t));
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
// Producto escalar de salida con SCReLU.
//
// Matematicamente identico al bucle original en int64: cada termino
// clamp(x,0,QA)^2 * w cabe en int32 y la suma se acumula por bloques de
// BLOQUE neuronas en un int32 que se vuelca a un int64. Al no haber
// desbordamiento en ningun punto el total es EXACTAMENTE el mismo, pero
// las multiplicaciones pasan a ser de 32 bits -- que NEON/AVX si
// vectorizan, a diferencia de las de 64 bits del bucle original.
//
// La ausencia de desbordamiento se comprueba en carga (g_rapido): hace
// falta 2*BLOQUE*QA*QA*max|l1w| <= INT32_MAX. Si unos pesos no cumplen,
// se usa el camino lento en int64 de siempre.
template <size_t H>
inline int64_t propagar(const int16_t* __restrict yo, const int16_t* __restrict riv,
                        const int16_t* __restrict w) {
    int64_t total = 0;
    for (size_t base = 0; base < H; base += BLOQUE) {
        int32_t s = 0;
        for (size_t j = 0; j < BLOQUE; ++j) {
            int32_t v = yo[base + j];
            v = v < 0 ? 0 : (v > QA ? QA : v);
            s += v * v * w[base + j];
        }
        for (size_t j = 0; j < BLOQUE; ++j) {
            int32_t u = riv[base + j];
            u = u < 0 ? 0 : (u > QA ? QA : u);
            s += u * u * w[H_MAX + base + j];
        }
        total += s;
    }
    return total;
}

// Camino de referencia en int64, valido para cualquier H y cualesquiera
// pesos. Se conserva como red de seguridad.
inline int64_t propagar_lento(const int16_t* yo, const int16_t* riv, const int16_t* w, size_t h) {
    int64_t suma = 0;
    for (size_t j = 0; j < h; ++j) {
        int64_t v = std::clamp<int64_t>(yo[j], 0, QA);
        suma += v * v * w[j];
        int64_t u = std::clamp<int64_t>(riv[j], 0, QA);
        suma += u * u * w[H_MAX + j];
    }
    return suma;
}

inline size_t bucket_de(int piezas) {
    if (g_red.buckets == 1) return 0;
    size_t divisor = (32 + g_red.buckets - 1) / g_red.buckets;
    size_t idx = (static_cast<size_t>(std::max(piezas, 2)) - 2) / divisor;
    return std::min(idx, g_red.buckets - 1);
}

}  // namespace

namespace {

// Logica de parseo compartida entre cargar desde archivo y cargar desde los
// bytes embebidos en el binario. `datos` ya debe contener el archivo
// completo (utiles + relleno de alineacion).
bool cargar_desde_bytes(const std::vector<uint8_t>& datos) {
    const size_t tam = datos.size();
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

    // Punteros crudos cacheados para los bucles calientes.
    g_l0w = g_red.l0w.data();
    g_l0b = g_red.l0b.data();
    g_l1w = g_red.l1w.data();
    g_l1b = g_red.l1b.data();
    g_h = g_red.h;

    // ¿Es exacto el producto escalar acumulado en int32? Hace falta que un
    // bloque entero de terminos quepa sin desbordar.
    int64_t max_abs = 0;
    for (int16_t v : g_red.l1w) {
        max_abs = std::max<int64_t>(max_abs, std::abs(static_cast<int>(v)));
    }
    const int64_t cota = 2LL * static_cast<int64_t>(BLOQUE) * QA * QA * max_abs;
    g_rapido = (cota <= INT32_MAX);

    g_desc = std::to_string(h) + " neuronas, " + std::to_string(b) + " output bucket(s)";
    // Los pesos cambiaron: el acumulador que hubiera de una red anterior ya
    // no vale nada. Se marca invalido; refresh() lo reconstruye (el llamador
    // en UciHandler.cpp hace ese refresh justo despues de un load() exitoso).
    g_acc_listo = false;
    std::cout << "info string NNUE: cargada red de " << g_desc << std::endl;
    return true;
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
    std::vector<uint8_t> datos(static_cast<size_t>(n));
    if (!f.read(reinterpret_cast<char*>(datos.data()), n)) {
        std::cout << "info string NNUE: error leyendo el archivo" << std::endl;
        return false;
    }
    return cargar_desde_bytes(datos);
}

bool load_embebida() {
    std::vector<uint8_t> datos(nnue_pesos_embebidos_bytes,
                                nnue_pesos_embebidos_bytes + nnue_pesos_embebidos_bytes_len);
    if (cargar_desde_bytes(datos)) {
        std::cout << "info string NNUE: red embebida cargada, activada por defecto" << std::endl;
        return true;
    }
    return false;
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
    actualizar_pieza<true>(pce, sq);
    ++g_piezas;
}

void on_remove_piece(int pce, int sq) {
    if (!g_loaded) return;
    actualizar_pieza<false>(pce, sq);
    --g_piezas;
}

void on_move_piece(int pce, int from, int to) {
    if (!g_loaded) return;
    size_t color, tipo;
    color_y_tipo(pce, color, tipo);
    const size_t sf = static_cast<size_t>(from ^ 56);
    const size_t st = static_cast<size_t>(to ^ 56);
    const size_t q0 = feature(0, color, tipo, sf);
    const size_t q1 = feature(1, color, tipo, sf);
    const size_t p0 = feature(0, color, tipo, st);
    const size_t p1 = feature(1, color, tipo, st);
    if (g_h == 512) {
        nucleo_mover<512>(g_acc[0], g_l0w + q0 * 512, g_l0w + p0 * 512);
        nucleo_mover<512>(g_acc[1], g_l0w + q1 * 512, g_l0w + p1 * 512);
    } else if (g_h == 256) {
        nucleo_mover<256>(g_acc[0], g_l0w + q0 * 256, g_l0w + p0 * 256);
        nucleo_mover<256>(g_acc[1], g_l0w + q1 * 256, g_l0w + p1 * 256);
    } else {
        nucleo_mover_gen(g_acc[0], g_l0w + q0 * g_h, g_l0w + p0 * g_h, g_h);
        nucleo_mover_gen(g_acc[1], g_l0w + q1 * g_h, g_l0w + p1 * g_h, g_h);
    }
    // El numero de piezas no cambia: una sale de `from` y entra en `to`.
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
    const int16_t* w = g_l1w + bk * 2 * H_MAX;

    // SCReLU: clamp(x, 0, QA)^2. El cuadrado deja la escala en QA^2*QB, por
    // eso despues se divide UNA vez entre QA para volver a QA*QB, que es la
    // escala en la que bullet guardo l1b.
    int64_t suma;
    if (g_rapido && g_h == 512) {
        suma = propagar<512>(yo, rival, w);
    } else if (g_rapido && g_h == 256) {
        suma = propagar<256>(yo, rival, w);
    } else {
        suma = propagar_lento(yo, rival, w, g_h);
    }

    const int64_t salida = suma / QA + g_l1b[bk];
    return static_cast<int>(std::lround(static_cast<double>(salida * SCALE) / (QA * QB)));
}

}  // namespace nnue
