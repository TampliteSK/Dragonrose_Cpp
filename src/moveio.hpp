// moveio.hpp

#ifndef MOVEIO_HPP
#define MOVEIO_HPP

#include <vector>
#include <string>
#include "movegen.hpp"
#include "Board.hpp"

std::string print_move(int move);
int parse_move(const Board *pos, std::string move_string);
void print_move_list(const std::vector<Move> move_list);
void print_move_list_compact(const std::vector<Move> move_list);

#endif // MOVEIO_HPP