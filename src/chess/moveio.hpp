// moveio.hpp

#ifndef MOVEIO_HPP
#define MOVEIO_HPP

#include "../StaticVector.hpp"
#include "movegen.hpp"
#include "Board.hpp"

#include <string>

std::string print_move(int move);
int parse_move(const Board *pos, std::string move_string);
void print_move_list(const StaticVector<Move, MAX_LEGAL_MOVES> move_list, bool verbose);

#endif // MOVEIO_HPP