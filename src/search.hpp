// search.hpp

#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <cstdint>
#include "ttable.hpp"
#include "Board.hpp"
#include "datatypes.hpp"

class Board;

typedef struct {
	uint64_t start_time;
	uint64_t stop_time;
	uint8_t depth;
	uint8_t seldepth;
	uint64_t nodes;

	bool timeset;
	uint16_t movestogo;
	bool quit;
	bool stopped;

	float fh; // beta cutoffs
	float fhf; // legal moves
	uint16_t null_cut;

} SearchInfo;

extern int LMR_reduction_table[MAX_DEPTH][280]; // [ply][move_num]

// Functions
void search_position(Board* pos, HashTable* table, SearchInfo* info);
void init_searchinfo(SearchInfo* info);
void init_LMR_table();

#endif // SEARCH_HPP