// bitboard.cpp

#include "bitboard.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>

// Diagonals and anti-diagonals
const int diagonals[64]{14, 13, 12, 11, 10, 9, 8, 7, 13, 12, 11, 10, 9, 8, 7, 6,
                        12, 11, 10, 9,  8,  7, 6, 5, 11, 10, 9,  8,  7, 6, 5, 4,
                        10, 9,  8,  7,  6,  5, 4, 3, 9,  8,  7,  6,  5, 4, 3, 2,
                        8,  7,  6,  5,  4,  3, 2, 1, 7,  6,  5,  4,  3, 2, 1, 0};

const int anti_diagonals[64]{0, 15, 14, 13, 12, 11, 10, 9,  1, 0, 15, 14, 13, 12, 11, 10,
                             2, 1,  0,  15, 14, 13, 12, 11, 3, 2, 1,  0,  15, 14, 13, 12,
                             4, 3,  2,  1,  0,  15, 14, 13, 5, 4, 3,  2,  1,  0,  15, 14,
                             6, 5,  4,  3,  2,  1,  0,  15, 7, 6, 5,  4,  3,  2,  1,  0};

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
            return 0ULL;  // King shield is useless when you're on the other side of the board
        }

        start_sq = FR2SQ(king_file - 1, king_rank - 1);
        end_sq = FR2SQ(king_file + 1, king_rank - 3);
        if (king_file == FILE_A) {
            start_sq = FR2SQ(king_file, king_rank - 1);
        }
        if (king_file == FILE_H) {
            end_sq = FR2SQ(king_file, king_rank - 3);
        }
    } else {
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
            } else {
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
[[nodiscard]] constexpr uint8_t dist_between_squares(uint8_t sq1, uint8_t sq2) {
    return std::abs(static_cast<int>(GET_FILE(sq1)) - GET_FILE(sq2)) +
           std::abs(static_cast<int>(GET_RANK(sq1)) - GET_RANK(sq2));
}