// attack.cpp

#include "attack.hpp"
#include "bitboard.hpp"
#include "movegen.hpp"

Bitboard get_piece_attacks(const Board *pos, uint8_t pce, uint8_t sq) {
    if (piece_type[pce] ==   PAWN) return pawn_attacks[piece_col[pce]][sq];
    if (piece_type[pce] == KNIGHT) return knight_attacks[sq];
    if (piece_type[pce] == BISHOP) return get_bishop_attacks(sq, pos->occupancies[BOTH]);
    if (piece_type[pce] ==   ROOK) return get_rook_attacks(sq, pos->occupancies[BOTH]);
    if (piece_type[pce] ==  QUEEN) return get_queen_attacks(sq, pos->occupancies[BOTH]);
    if (piece_type[pce] ==   KING) return king_attacks[sq];
    return 0ULL;
}

bool is_move_attack(const Board *pos, int move) {
    uint8_t piece = get_move_piece(move);
    uint8_t target_sq = get_move_target(move);
    // Generate the attacks at the target sq, as if the piece is already there
    return get_piece_attacks(pos, piece, target_sq) & pos->occupancies[pos->side ^ 1];
}

// Replace with incremental version in board
void get_all_attacks(const Board *pos, uint8_t side, Bitboard* list, int* attackers, bool king_included) {
    Bitboard copy = pos->occupancies[side];
    int count = 0;

    while (copy) {
        uint8_t sq = pop_ls1b(copy);
        uint8_t pce = pos->pieces[sq];
        if (king_included || piece_type[pce] != KING) {
            list[count] = get_piece_attacks(pos, pce, sq);
            attackers[count] = pce;
            count++;
        }
    }
}