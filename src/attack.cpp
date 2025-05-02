// attack.cpp

#include <iostream>
#include "attack.hpp"
#include "attackgen.hpp"
#include "Board.hpp"
#include "bitboard.hpp"
#include "movegen.hpp"

// Check if the current square is attacked by a given side
bool is_square_attacked(const Board *pos, uint8_t sq, uint8_t side) {

    // Pawns (flip the direction of the atacks)
    if ((side == WHITE) && (pawn_attacks[BLACK][sq] & pos->bitboards[wP])) return true;
    if ((side == BLACK) && (pawn_attacks[WHITE][sq] & pos->bitboards[bP])) return true;

    // Knights and Kings
    if (knight_attacks[sq] & ((side == WHITE) ? pos->bitboards[wN] : pos->bitboards[bN])) return true;
    if (king_attacks[sq]   & ((side == WHITE) ? pos->bitboards[wK] : pos->bitboards[bK])) return true;

    // Bishops, Rooks and Queens
    if (get_bishop_attacks(sq, pos->occupancies[BOTH]) & ((side == WHITE) ? pos->bitboards[wB] : pos->bitboards[bB])) return true;
    if (get_rook_attacks(sq, pos->occupancies[BOTH])   & ((side == WHITE) ? pos->bitboards[wR] : pos->bitboards[bR])) return true;
    if (get_queen_attacks(sq, pos->occupancies[BOTH])  & ((side == WHITE) ? pos->bitboards[wQ] : pos->bitboards[bQ])) return true;

    return false;
}

// Check if the current square is attacked by a given side, weighted by the reciprocal of the piece:
// Control by a piece = (9 / V)^0.75 rounded to 2 d.p.
// where V is the traditional value of pieces, and kings are considered as 10 points
double count_attacks(const Board *pos, uint8_t sq, uint8_t side) {
    double attacks = 0;

    if (side == WHITE) {
        attacks += 5.20 * count_bits(pawn_attacks[BLACK][sq] & pos->bitboards[wP]);                        // Pawns
        attacks += 2.28 * count_bits(knight_attacks[sq] & pos->bitboards[wN]);                             // Knights
        attacks += 2.28 * count_bits(get_bishop_attacks(sq, pos->occupancies[BOTH]) & pos->bitboards[wB]); // Bishops
        attacks += 1.55 * count_bits(get_rook_attacks(sq, pos->occupancies[BOTH]) & pos->bitboards[wR]);   // Rooks
        attacks += 1 * count_bits(get_queen_attacks(sq, pos->occupancies[BOTH]) & pos->bitboards[wQ]);     // Queens
        attacks += 0.92 * count_bits(king_attacks[sq] & pos->bitboards[wK]);                               // Kings
    }
    else {
        attacks += 5.20 * count_bits(pawn_attacks[WHITE][sq] & pos->bitboards[bP]);                        // Pawns
        attacks += 2.28 * count_bits(knight_attacks[sq] & pos->bitboards[bN]);                             // Knights
        attacks += 2.28 * count_bits(get_bishop_attacks(sq, pos->occupancies[BOTH]) & pos->bitboards[bB]); // Bishops
        attacks += 1.55 * count_bits(get_rook_attacks(sq, pos->occupancies[BOTH]) & pos->bitboards[bR]);   // Rooks
        attacks += 1 * count_bits(get_queen_attacks(sq, pos->occupancies[BOTH]) & pos->bitboards[bQ]);     // Queens
        attacks += 0.92 * count_bits(king_attacks[sq] & pos->bitboards[bK]);                               // Kings
    }

    return attacks;
}

double get_square_control(const Board *pos, uint8_t sq, uint8_t side) {
    return count_attacks(pos, sq, side) - count_attacks(pos, sq, side ^ 1);
}

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
Bitboard get_all_attacks(const Board *pos, uint8_t side, bool king_included) {
    Bitboard copy = pos->occupancies[side];
    Bitboard attacks = 0ULL;
    while (copy) {
        uint8_t sq = pop_ls1b(copy);
        uint8_t pce = pos->pieces[sq];
        if (king_included || piece_type[pce] != KING) {
            attacks |= get_piece_attacks(pos, pce, sq);
        }
    }
    return attacks;
}