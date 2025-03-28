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

int main(int argc, char* argv[]) {
	init_all();

	Board* pos = new Board(START_POS);
	
	SearchInfo* info = (SearchInfo*)malloc(sizeof(SearchInfo));
	init_searchinfo(info);

	HashTable* hash_table = (HashTable*)malloc(sizeof(HashTable)); // Global hash
	hash_table->pTable = NULL;

	info->depth = 5;
	info->start_time = get_time_ms();
	info->stop_time = get_time_ms() * 2;

	init_hash_table(hash_table, 16);
	search_position(pos, hash_table, info);

	return 0;
}