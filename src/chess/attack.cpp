// attack.cpp

#include "attack.hpp"

#include "movegen.hpp"

bool is_move_attack(const Board& pos, int move) {
    uint8_t piece = get_move_piece(move);
    uint8_t target_sq = get_move_target(move);
    // Generate the attacks at the target sq, as if the piece is already there
    return get_piece_attacks(pos, piece, target_sq) & pos.occupancies[pos.side ^ 1];
}
