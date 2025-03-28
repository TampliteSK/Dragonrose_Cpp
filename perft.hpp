// perft.hpp

#ifndef PERFT_HPP
#define PERFT_HPP

#include <cstdint>
#include "Board.hpp"

uint64_t run_perft(Board *pos, uint8_t depth, bool print_info);

#endif // PERFT_HPP