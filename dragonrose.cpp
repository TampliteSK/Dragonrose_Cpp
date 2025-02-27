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

	Board *pos = new Board();
	pos->parse_fen(START_POS);
	
	/*
	std::vector<Move> move_list;
	generate_moves(pos, move_list);
	sort_moves(pos, move_list);
	print_move_list(move_list);

	bool status = false;
	status = make_move(pos, move_list[0].move);
	pos->print_board();
	std::cout << "Is move legal? " << status << std::endl;
	take_move(pos);
	pos->print_board();
	make_move(pos, move_list[1].move);
	std::cout << "Is move legal? " << status << std::endl;
	pos->print_board();
	*/

	uint64_t nodes = 0;
	perft_test(pos, 2, nodes);

	return 0;
}