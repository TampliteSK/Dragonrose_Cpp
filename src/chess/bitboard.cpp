// bitboard.cpp

#include "../datatypes.hpp"
#include "bitboard.hpp"
#include "Board.hpp"

#include <bit>
#include <bitset>
#include <cstdint>
#include <algorithm>
#include <iostream>

// Diagonals and anti-diagonals
const int diagonals[64]{
    14, 13, 12, 11, 10,  9,  8,  7,
    13, 12, 11, 10,  9,  8,  7,  6,
    12, 11, 10,  9,  8,  7,  6,  5,
    11, 10,  9,  8,  7,  6,  5,  4,
    10,  9,  8,  7,  6,  5,  4,  3,
     9,  8,  7,  6,  5,  4,  3,  2,
     8,  7,  6,  5,  4,  3,  2,  1,
     7,  6,  5,  4,  3,  2,  1,  0
};

const int anti_diagonals[64]{
     0, 15, 14, 13, 12, 11, 10,  9,
     1,  0, 15, 14, 13, 12, 11, 10,
     2,  1,  0, 15, 14, 13, 12, 11,
     3,  2,  1,  0, 15, 14, 13, 12,
     4,  3,  2,  1,  0, 15, 14, 13,
     5,  4,  3,  2,  1,  0, 15, 14,
     6,  5,  4,  3,  2,  1,  0, 15,
     7,  6,  5,  4,  3,  2,  1,  0
};

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

Bitboard generate_shield_zone(uint8_t king_sq, uint8_t col) {

    /*
    --------
    --------
    --------
    --------
    -----xxx
    -----xxx
    -----xxx
    ------K-
    */

    Bitboard shield_zone = 0ULL;
    uint8_t king_file = GET_FILE(king_sq);
    uint8_t king_rank = GET_RANK(king_sq);

    uint8_t start_sq = 0, end_sq = 0;
    if (col == WHITE) {
        if (king_rank <= RANK_5) {
            return 0ULL; // King shield is useless when you're on the other side of the board
        }

        start_sq = FR2SQ(king_file - 1, king_rank - 1);
        end_sq = FR2SQ(king_file + 1, king_rank - 3);
        if (king_file == FILE_A) {
            start_sq = FR2SQ(king_file, king_rank - 1);
        }
        if (king_file == FILE_H) {
            end_sq = FR2SQ(king_file, king_rank - 3);
        }
    }
    else {
        if (king_rank >= RANK_4) {
            return 0ULL;
        }

        start_sq = FR2SQ(king_file - 1, king_rank + 1);
        end_sq = FR2SQ(king_file + 1, king_rank + 3);
        if (king_file == FILE_A) {
            start_sq = FR2SQ(king_file, king_rank + 1);
        }
        if (king_file == FILE_H) {
            end_sq = FR2SQ(king_file, king_rank + 3);
        }
    }
    
    shield_zone = bits_between_squares(start_sq, end_sq);

    return shield_zone;
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