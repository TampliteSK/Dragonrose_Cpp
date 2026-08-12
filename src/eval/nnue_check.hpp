// nnue_check.hpp
//
// Verificacion de correctness del acumulador incremental: recorre
// exhaustivamente el arbol de jugadas legales (igual que perft) y en cada
// nodo comprueba que el acumulador mantenido por make_move/take_move
// coincide EXACTO (bit a bit) con un recalculo completo desde cero.
// Cualquier divergencia es un bug critico en los ganchos de makemove.cpp.

#ifndef NNUE_CHECK_HPP
#define NNUE_CHECK_HPP

#include <cstdint>

#include "../chess/Board.hpp"

namespace nnue_check {

struct Resultado {
    uint64_t nodos = 0;   // posiciones visitadas (tras make_move)
    uint64_t fallos = 0;  // de esas, cuantas no coincidieron con el recalculo
};

// Recorre el arbol de jugadas legales desde `pos` hasta profundidad
// `profundidad`, verificando consistencia tras cada make_move y tras cada
// take_move (para detectar tambien bugs de "undo"). Deja `pos` como estaba
// al entrar (recorrido make/unmake balanceado, como perft).
Resultado verificar(Board& pos, int profundidad);

}  // namespace nnue_check

#endif  // NNUE_CHECK_HPP
