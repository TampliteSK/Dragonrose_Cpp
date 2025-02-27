// perft.hpp

#ifndef PERFT_HPP
#define PERFT_HPP

#include <cstdint>
#include "Board.hpp"

void perft_test(Board *pos, int depth, uint64_t& nodes);

#endif // PERFT_HPP