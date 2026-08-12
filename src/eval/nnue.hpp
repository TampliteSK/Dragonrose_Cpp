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

bool is_loaded();

// Activa/desactiva el uso de la red en evaluate_pos(). Apagado por
// defecto: sin un `setoption name UseNNUE value true` explicito el motor
// se comporta exactamente igual que antes.
void set_enabled(bool on);
bool is_enabled();

// Descripcion legible de la red cargada, para `info string`.
std::string description();

// Evaluacion en centipeones, relativa al lado que mueve.
// Precondicion: is_loaded().
int evaluate(const Board& pos);

}  // namespace nnue

#endif  // NNUE_HPP
