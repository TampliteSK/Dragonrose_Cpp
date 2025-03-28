// dragonrose.cpp

#include <iostream>
#include <vector>
#include "Board.hpp"
#include "makemove.hpp"
#include "init.hpp"
#include "bitboard.hpp"
#include "movegen.hpp"
#include "moveio.hpp"
#include "perft.hpp"

#define TEST1 "r3kn1r/p3b1p1/2pq1p2/2p1p2p/6bP/1P1P1NP1/PBPNQP2/2K1R2R b kq - 4 15"

int main(int argc, char* argv[]) {
	init_all();

	Board* pos = new Board(START_POS);
	run_perft(pos, 5, true);

	return 0;
}