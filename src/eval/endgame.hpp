// endgame.hpp

#ifndef ENDGAME_HPP
#define ENDGAME_HPP

#include "../chess/Board.hpp"
#include "../datatypes.hpp"

int8_t endgame_noise(unsigned int seed, uint8_t width);
bool check_material_draw(const Board* pos);
uint16_t check_material_win(const Board* pos);

#endif // ENDGAME_HPP