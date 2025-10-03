// bitboard.hpp

#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include <bit>

#include "../datatypes.hpp"
#include "Board.hpp"

// set/get/pop bit macros
#define GET_BIT(bitboard, square) ((bitboard) & (1ULL << (square)))
#define SET_BIT(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define CLR_BIT(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

#define IS_LIGHT_SQ(sq) (bool)((1ULL << (sq)) & LIGHT_SQUARES)
#define IS_DARK_SQ(sq) !(bool)((1ULL << (sq)) & LIGHT_SQUARES)

#define SAME_DIAGONAL(sq1, sq2) \
    ((diagonals[(sq1)] == diagonals[(sq2)]) || (anti_diagonals[(sq1)] == anti_diagonals[(sq2)]))

const Bitboard LIGHT_SQUARES =
    0xAA55AA55AA55AA55ULL;                           // Bits corresponding to light squares are set
const Bitboard BOTTOM_HALF = 0x00000000FFFFFFFFULL;  // A1 ~ H4 set
const Bitboard TOP_HALF = 0xFFFFFFFF00000000ULL;     // A5 ~ H8 set

extern const int diagonals[64];
extern const int anti_diagonals[64];

/*
    Bit Manipulation
*/

// Clears the least significant set bit and returns its index (0 - 63)
static inline uint8_t pop_ls1b(Bitboard &bb) {
    uint8_t index = __builtin_ctzll(bb);  // Use gcc/clang intrinsic
    bb &= bb - 1;
    return index;
}

// Counts the number of set bits in a bitboard
static inline uint8_t count_bits(Bitboard bb) { return (uint8_t)std::popcount(bb); }

// Bitboard functions
// Mask a bitboard with bits between 2 given squares set
static inline Bitboard bits_between_squares(uint8_t sq1, uint8_t sq2) {
    Bitboard mask = 0ULL;
    uint8_t rank1 = GET_RANK(sq1);
    uint8_t rank2 = GET_RANK(sq2);
    uint8_t file1 = GET_FILE(sq1);
    uint8_t file2 = GET_FILE(sq2);

    bool same_rank = rank1 == rank2;
    bool same_file = file1 == file2;

    // 0-D case
    if (sq1 == sq2) {
        return mask;
    }
    // 1-D cases
    else if (same_rank ^ same_file) {
        if (same_rank) {
            for (int sq = std::min(sq1, sq2); sq <= std::max(sq1, sq2); ++sq) {
                SET_BIT(mask, sq);
            }
        } else {
            for (int sq = std::min(sq1, sq2); sq <= std::max(sq1, sq2); sq += 8) {
                SET_BIT(mask, sq);
            }
        }
    }
    // 2-D case
    else {
        for (int rank = std::min(rank1, rank2); rank <= std::max(rank1, rank2); ++rank) {
            for (int file = std::min(file1, file2); file <= std::max(file1, file2); ++file) {
                uint8_t sq = FR2SQ(file, rank);
                SET_BIT(mask, sq);
            }
        }
    }

    return mask;
}

Bitboard generate_shield_zone(uint8_t king_sq, uint8_t col);
void print_bitboard(Bitboard board);
uint8_t dist_between_squares(uint8_t sq1, uint8_t sq2);

#endif  // BITBOARD_HPP