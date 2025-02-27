// attack.cpp

#include <iostream>
#include "attack.hpp"
#include "attackgen.hpp"
#include "Board.hpp"
#include "bitboard.hpp"

// Check if the current square is attacked by a given side
bool is_square_attacked(const Board *pos, uint8_t sq, uint8_t side) {

    // Pawns
    if ((side == WHITE) && (pawn_attacks[BLACK][sq] & pos->get_bitboard(wP))) return true;
    if ((side == BLACK) && (pawn_attacks[WHITE][sq] & pos->get_bitboard(bP))) return true;

    // Knights and Kings
    if (knight_attacks[sq] & ((side == WHITE) ? pos->get_bitboard(wN) : pos->get_bitboard(bN))) return true;
    if (king_attacks[sq]   & ((side == WHITE) ? pos->get_bitboard(wK) : pos->get_bitboard(bK))) return true;

    // Bishops, Rooks and Queens
    if (get_bishop_attacks(sq, pos->get_occupancy(BOTH)) & ((side == WHITE) ? pos->get_bitboard(wB) : pos->get_bitboard(bB))) return true;
    if (get_rook_attacks(sq, pos->get_occupancy(BOTH))   & ((side == WHITE) ? pos->get_bitboard(wR) : pos->get_bitboard(bR))) return true;
    if (get_queen_attacks(sq, pos->get_occupancy(BOTH))  & ((side == WHITE) ? pos->get_bitboard(wQ) : pos->get_bitboard(bQ))) return true;

    return false;
}

// Check if the current square is attacked by a given side
uint8_t count_attacks(const Board *pos, uint8_t sq, uint8_t side) {
    uint8_t attacks = 0;

    if (side == WHITE) {
        attacks += count_bits(pawn_attacks[BLACK][sq] & pos->get_bitboard(wP));                         // Pawns
        attacks += count_bits(knight_attacks[sq] & pos->get_bitboard(wN));                              // Knights
        attacks += count_bits(king_attacks[sq] & pos->get_bitboard(wK));                                // Kings
        attacks += count_bits(get_bishop_attacks(sq, pos->get_occupancy(BOTH) & pos->get_bitboard(wB))); // Bishops
        attacks += count_bits(get_rook_attacks(sq, pos->get_occupancy(BOTH) & pos->get_bitboard(wR)));   // Rooks
        attacks += count_bits(get_queen_attacks(sq, pos->get_occupancy(BOTH) & pos->get_bitboard(wQ)));  // Queens
    }
    else {
        attacks += count_bits(pawn_attacks[WHITE][sq] & pos->get_bitboard(bP));                         // Pawns
        attacks += count_bits(knight_attacks[sq] & pos->get_bitboard(bN));                              // Knights
        attacks += count_bits(king_attacks[sq] & pos->get_bitboard(bK));                                // Kings
        attacks += count_bits(get_bishop_attacks(sq, pos->get_occupancy(BOTH)) & pos->get_bitboard(bB)); // Bishops
        attacks += count_bits(get_rook_attacks(sq, pos->get_occupancy(BOTH)) & pos->get_bitboard(bR));   // Rooks
        attacks += count_bits(get_queen_attacks(sq, pos->get_occupancy(BOTH) & pos->get_bitboard(bQ)));  // Queens
    }

    return attacks;
}

bool is_square_controlled(const Board *pos, uint8_t sq, uint8_t side) {
    return count_attacks(pos, sq, side) > count_attacks(pos, sq, side ^ 1);
}

Bitboard get_piece_attacks(const Board *pos, uint8_t pce, uint8_t sq) {
    if (piece_type[pce] ==   PAWN) return pawn_attacks[piece_col[pce]][sq];
    if (piece_type[pce] == KNIGHT) return knight_attacks[sq];
    if (piece_type[pce] == BISHOP) return get_bishop_attacks(sq, pos->get_occupancy(BOTH));
    if (piece_type[pce] ==   ROOK) return get_rook_attacks(sq, pos->get_occupancy(BOTH));
    if (piece_type[pce] ==  QUEEN) return get_queen_attacks(sq, pos->get_occupancy(BOTH));
    if (piece_type[pce] ==   KING) return king_attacks[sq];
}

Bitboard get_all_attacks(const Board *pos, uint8_t side, bool king_included) {
    Bitboard copy = pos->get_occupancy(side);
    Bitboard attacks = 0ULL;
    while (copy) {
        uint8_t sq = pop_ls1b(copy);
        uint8_t pce = pos->get_piece(sq);
        if (king_included || piece_type[pce] != KING) {
            attacks |= get_piece_attacks(pos, pce, sq);
        }
    }
    return attacks;
}