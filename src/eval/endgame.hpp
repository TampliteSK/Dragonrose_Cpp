// endgame.hpp

#ifndef ENDGAME_HPP
#define ENDGAME_HPP

#include "../chess/Board.hpp"
#include "../datatypes.hpp"

int16_t endgame_noise(unsigned int seed, uint8_t width);
bool check_material_draw(const Board& pos, const uint8_t phase);
// uint16_t check_material_win(const Board& pos);

#endif  // ENDGAME_HPP