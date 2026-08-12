// nnue.hpp
//
// Inferencia NNUE en formato "bullet" (https://github.com/jw1912/bullet),
// compatible con las redes entrenadas para Mittens.
//
//     (768 -> H)x2 -> 1,  doble perspectiva,  activacion SCReLU
//
// Entrada 768 = 2 colores x 6 tipos de pieza x 64 casillas. Hay DOS
// acumuladores (uno por perspectiva); la capa de salida recibe
// [perspectiva_del_que_mueve | la_del_rival], asi que la red aprende sola
// el valor del turno y su salida ya viene relativa al lado que mueve --
// la misma convencion que devuelve evaluate_pos().
//
// FASE 1: el acumulador se reconstruye ENTERO en cada evaluacion (no hay
// actualizacion incremental en make/unmake todavia). Es correcto pero
// cuesta nps; la version incremental es trabajo posterior.

#ifndef NNUE_HPP
#define NNUE_HPP

#include <string>

#include "../chess/Board.hpp"

namespace nnue {

// Carga una red desde disco. Devuelve false (y deja la red anterior
// intacta) si el archivo no corresponde a ninguna arquitectura conocida o
// si el relleno final no es la firma esperada.
bool load(const std::string& path);

// Carga la red embebida en el binario (pesos de produccion de Mittens,
// 768->512x2->1). No depende de ningun archivo externo.
bool load_embebida();

bool is_loaded();

// Activa/desactiva el uso de la red en evaluate_pos(). Apagado por
// defecto: sin un `setoption name UseNNUE value true` explicito el motor
// se comporta exactamente igual que antes.
void set_enabled(bool on);
bool is_enabled();

// Descripcion legible de la red cargada, para `info string`.
std::string description();

// Evaluacion en centipeones, relativa al lado que mueve. Usa el acumulador
// mantenido incrementalmente (ver refresh()/on_add_piece()/on_remove_piece());
// no recalcula nada desde cero.
// Precondicion: is_loaded().
int evaluate(const Board& pos);

// --- Acumulador incremental (fase 2) ---
//
// El acumulador ya no se reconstruye en cada evaluate(): se mantiene al dia
// via ganchos llamados desde clear_piece/add_piece/move_piece en
// makemove.cpp, exactamente igual que el hash Zobrist incremental que ya
// existia en ese archivo (XOR / suma-resta son ambos autoinversos, asi que
// los mismos ganchos sirven para make_move Y take_move sin pila aparte).
//
// refresh() reconstruye el acumulador entero desde `pos` y debe llamarse
// una vez por cada posicion "de base" (tras parse_fen, tras cargar una red
// nueva, o al activar UseNNUE); a partir de ahi make_move/take_move lo
// mantienen solos. No hace nada si no hay red cargada.
void refresh(const Board& pos);

// Ganchos de mantenimiento incremental. No-op si no hay red cargada (una
// sola comprobacion de bool, mismo coste que antes cuando NNUE esta
// apagada). `pce` en la codificacion de Dragonrose (wP..bK), `sq` en su
// convencion de casillas (a8=0..h1=63).
void on_add_piece(int pce, int sq);
void on_remove_piece(int pce, int sq);

// Solo para verificacion/tests: compara el acumulador mantenido
// incrementalmente contra un recalculo completo desde `pos`. true si no
// hay red cargada (nada que comprobar).
bool consistente_con_recalculo(const Board& pos);

}  // namespace nnue

#endif  // NNUE_HPP
