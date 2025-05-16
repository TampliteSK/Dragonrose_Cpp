// bitboard.hpp

#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "datatypes.hpp"

// set/get/pop bit macros
#define GET_BIT(bitboard, square) ((bitboard) & (1ULL << (square)))
#define SET_BIT(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define CLR_BIT(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

#define IS_LIGHT_SQ(sq) (bool)( (1ULL << (sq)) & LIGHT_SQUARES )
#define IS_DARK_SQ(sq) !(bool)( (1ULL << (sq)) & LIGHT_SQUARES )

const Bitboard LIGHT_SQUARES = 0xAA55AA55AA55AA55ULL; // Bits corresponding to light squares are set
const Bitboard BOTTOM_HALF = 0x00000000FFFFFFFFULL; // A1 ~ H4 set
const Bitboard TOP_HALF = 0xFFFFFFFF00000000ULL; // A5 ~ H8 set


// Bitboard functions
uint8_t pop_ls1b(Bitboard& bb);
uint8_t count_bits(Bitboard bb);
Bitboard bits_between_squares(uint8_t sq1, uint8_t sq2);
Bitboard generate_shield_zone(uint8_t king_sq, uint8_t col);
void print_bitboard(Bitboard board);
uint8_t dist_between_squares(uint8_t sq1, uint8_t sq2);

#endif // BITBOARD_HPP