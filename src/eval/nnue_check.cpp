// nnue_check.cpp -- ver nnue_check.hpp

#include "nnue_check.hpp"

#include <iostream>

#include "../chess/makemove.hpp"
#include "../chess/moveio.hpp"
#include "../chess/movegen.hpp"
#include "nnue.hpp"

namespace nnue_check {

namespace {

// Limite de mensajes de fallo impresos, para no inundar la salida si algo
// esta realmente roto.
constexpr uint64_t MAX_FALLOS_IMPRESOS = 20;

void revisar_nodo(Board& pos, Resultado& r, const char* momento) {
    ++r.nodos;
    if (!nnue::consistente_con_recalculo(pos)) {
        ++r.fallos;
        if (r.fallos <= MAX_FALLOS_IMPRESOS) {
            std::cout << "info string NNUE CHECK: divergencia tras " << momento << " en fen "
                      << "(usa 'print' o revisa move_history para reproducir)" << std::endl;
        }
    }
}

void recorrer(Board& pos, int profundidad, Resultado& r) {
    if (profundidad == 0) return;

    MoveList move_list;
    generate_moves(pos, move_list, false);

    for (int i = 0; i < (int)move_list.length; ++i) {
        int move = move_list.moves[i].move;
        if (!make_move(pos, move)) continue;  // ilegal, make_move ya deshizo

        revisar_nodo(pos, r, "make_move");
        recorrer(pos, profundidad - 1, r);

        take_move(pos);
        revisar_nodo(pos, r, "take_move");
    }
}

}  // namespace

Resultado verificar(Board& pos, int profundidad) {
    Resultado r;
    // Punto de partida: el acumulador debe coincidir ya con el recalculo
    // (parse_fen llama a nnue::refresh()).
    revisar_nodo(pos, r, "posicion inicial");
    recorrer(pos, profundidad, r);
    return r;
}

}  // namespace nnue_check
