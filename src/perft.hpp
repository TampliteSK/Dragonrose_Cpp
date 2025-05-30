// perft.hpp

#ifndef PERFT_HPP
#define PERFT_HPP

#include <cstdint>
#include "Board.hpp"

// Common test positions
/*
	Startpos - Depth 7 passed
	Kiwipete - Depth 5 passed
	CPW_Pos3 - Depth 7 passed
	CPW_Pos4 - Depth 6 passed
	CPW_Pos5 - Depth 6 passed (with webperft)
	CPW_Pos6 - Depth 6 passed

	Big thanks to Analog Hors for her webperft tool
*/
#define KIWIPETE "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"
#define CPW_POS3 "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
#define CPW_POS4 "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"
#define CPW_POS5 "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
#define CPW_POS6 "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"

uint64_t run_perft(Board *pos, uint8_t depth, bool print_info);

#endif // PERFT_HPP