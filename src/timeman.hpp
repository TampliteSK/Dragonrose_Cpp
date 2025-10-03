// timeman.hpp

#ifndef TIMEMAN_HPP
#define TIMEMAN_HPP

#include "Board.hpp"

uint64_t get_time_ms();
int allocate_time(const Board *pos, int time);

#endif  // TIMEMAN_HPP