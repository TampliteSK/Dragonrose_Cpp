// bitboard.cpp

#include <bit>
#include <bitset>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include "datatypes.hpp"
#include "bitboard.hpp"
#include "Board.hpp"

/*
    Bit Manipulation
*/

// Clears the least significant set bit and returns its index (0 - 63)
uint8_t pop_ls1b(Bitboard& bb) {
    uint8_t index = __builtin_ctzll(bb); // Use gcc/clang intrinsic
    bb &= ~1ULL << index;
    return index;
}

// Counts the number of set bits in a bitboard
uint8_t count_bits(Bitboard bb) {
    return (uint8_t)std::popcount(bb);
}

// Mask a bitboard with bits between 2 given squares set
Bitboard bits_between_squares(uint8_t sq1, uint8_t sq2) {

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
        }
        else {
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

/*
    Misc
*/

void print_bitboard(Bitboard board) {

    std::cout << "\n";

    for (int rank = RANK_8; rank <= RANK_1; ++rank) {
        std::cout << 8 - rank << "   ";
        for (int file = FILE_A; file <= FILE_H; ++file) {
            uint8_t sq = FR2SQ(file, rank);
            if (GET_BIT(board, sq)) {
                std::cout << "X";
            }
            else {
                std::cout << "-";
            }
        }
        std::cout << "\n";
    }

    std::cout << "    ABCDEFGH\n\n";
    std::cout << "Bits set: " << (int)count_bits(board) << "\n";
    std::cout << "\n";

}

// Manhattan distance
uint8_t dist_between_squares(uint8_t sq1, uint8_t sq2) {
    return abs(GET_FILE(sq1) - GET_FILE(sq2)) + abs(GET_RANK(sq1) - GET_RANK(sq2));
}
