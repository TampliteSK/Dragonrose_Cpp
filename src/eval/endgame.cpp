// endgame.cpp

#include "endgame.hpp"

#include <random>

#include "../chess/bitboard.hpp"
#include "../datatypes.hpp"
#include "evaluate.hpp"

// Returns an integer between -width ~ width cp as a draw score
// This makes the engine prefer positions with higher mobility (more likely to get higher scores)
int8_t endgame_noise(unsigned int seed, uint8_t width) {
    std::mt19937_64 mt(seed);  // 64-bit Mersenne Twister
    std::uniform_int_distribution<int> distribution(-width, width);
    return distribution(mt);
}

// Determines if the position is a drawn endgame (exc. pawns)
bool check_material_draw(const Board *pos, uint8_t phase) {
    constexpr uint16_t draw_threshold = 365;  // Value of a bishop in the opening

    // Exception to the rule
    if (count_bits(pos->occupancies[BOTH]) == 4) {
        // K + 2N v K
        if (pos->piece_num[wN] == 2 || pos->piece_num[bN] == 2) {
            return true;
        }
    }

    // General case
    return abs(count_material(pos, phase)) < draw_threshold;
}