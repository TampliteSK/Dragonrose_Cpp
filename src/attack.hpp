// attack.hpp

#ifndef ATTACK_HPP
#define ATTACK_HPP

#include <cstdint>
#include "Board.hpp"

bool is_square_attacked(const Board *pos, uint8_t sq, uint8_t side);

#endif // ATTACK_HPP