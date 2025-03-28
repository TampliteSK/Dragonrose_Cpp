// bitboard.hpp

#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "datatypes.hpp"

// set/get/pop bit macros
#define GET_BIT(bitboard, square) ((bitboard) & (1ULL << (square)))
#define SET_BIT(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define CLR_BIT(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

// Bitboard functions
uint8_t pop_ls1b(Bitboard& bb);
uint8_t count_bits(Bitboard bb);
Bitboard bits_between_squares(uint8_t sq1, uint8_t sq2);
void print_bitboard(Bitboard board);
uint8_t dist_between_squares(uint8_t sq1, uint8_t sq2);

#endif // BITBOARD_HPP