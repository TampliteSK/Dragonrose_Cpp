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
#include "search.hpp"
#include "timeman.hpp"
#include "ttable.hpp"
#include "evaluate.hpp"

int main(int argc, char* argv[]) {
	init_all();

	Board* pos = new Board(START_POS);
	
	SearchInfo* info = (SearchInfo*)malloc(sizeof(SearchInfo));
	init_searchinfo(info);

	HashTable* hash_table = (HashTable*)malloc(sizeof(HashTable)); // Global hash
	hash_table->pTable = NULL;

	info->depth = 8;
	info->start_time = get_time_ms();
	info->stop_time = get_time_ms() * 2;

	init_hash_table(hash_table, 16);
	search_position(pos, hash_table, info);

	/*
	std::vector<Move> list;
	generate_moves(pos, list, false);

	for (int i = 0; i < list.size(); ++i) {
		Board* copy = pos->clone();

		if (!make_move(copy, list[i].move)) {
			continue;
		}

		// copy->print_board();
		std::cout << "Startpos after " << print_move(list[i].move) << " | Eval: " << evaluate_pos(copy) << "\n";

		delete copy;
	}
	*/

	return 0;
}