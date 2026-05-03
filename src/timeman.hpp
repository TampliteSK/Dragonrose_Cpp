// timeman.hpp

#ifndef TIMEMAN_HPP
#define TIMEMAN_HPP

#include "Board.hpp"

struct Phase { 
    int limit; 
    int weight; 
};

uint64_t get_time_ms();
int allocate_time(const Board& pos, int base_time, int inc);

#endif  // TIMEMAN_HPP