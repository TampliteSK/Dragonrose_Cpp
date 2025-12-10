// moveio.hpp

#ifndef MOVEIO_HPP
#define MOVEIO_HPP

#include <string>

#include "../StaticVector.hpp"
#include "Board.hpp"
#include "movegen.hpp"

std::string print_move(int move);
int parse_move(const Board& pos, std::string move_string);
void print_move_list(const MoveList move_list, bool verbose);

#endif  // MOVEIO_HPP