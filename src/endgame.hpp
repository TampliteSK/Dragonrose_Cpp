// endgame.hpp

#include "datatypes.hpp"

#ifndef ENDGAME_HPP
#define ENDGAME_HPP

#include "Board.hpp"

int8_t endgame_noise(unsigned int seed, uint8_t width);
bool check_material_draw(const Board* pos);
uint16_t check_material_win(const Board* pos);

#endif // ENDGAME_HPP