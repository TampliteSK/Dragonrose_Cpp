// attack.hpp

#ifndef ATTACK_HPP
#define ATTACK_HPP

#include <cstdint>
#include "Board.hpp"

bool is_square_attacked(const Board *pos, uint8_t sq, uint8_t side);
Bitboard get_all_attacks(const Board* pos, uint8_t side, bool king_included);

#endif // ATTACK_HPP