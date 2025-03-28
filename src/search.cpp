// search.cpp

#include "search.hpp"
#include "datatypes.hpp"
#include "movegen.hpp"
#include "ttable.hpp"
#include "timeman.hpp"

/*
	Helper functions
*/

// Check if the time is up, if there's an interrupt from GUI
static void check_time(SearchInfo* info) {
	if (info->timeset && (get_time_ms() > info->stop_time)) {
		info->stopped = true;
	}
}

/*
static bool check_repetition(const Board* pos) {
	for (int index = pos->get_his_ply() - pos->get_fifty_move(); index < pos->get_his_ply() - 1; ++index) {
		if (pos->get_hash_key() == pos->history[index].posKey) {
			return true;
		}
	}
	return false;
}
*/

static void clear_search_vars(Board* pos, HashTable* table, SearchInfo* info) {

	for (int index = 0; index < 13; ++index) {
		for (int index2 = 0; index2 < 64; ++index2) {
			// pos->searchHistory[index][index2] = 40000;
		}
	}

	for (int index = 0; index < 2; ++index) {
		for (int index2 = 0; index2 < MAX_DEPTH; ++index2) {
			// pos->searchKillers[index][index2] = 40000;
		}
	}

	table->overwrite = 0;
	table->hit = 0;
	table->cut = 0;
	table->table_age++;
	pos->set_ply(0);

	info->stopped = false;
	info->nodes = 0;
	info->fh = 0;
	info->fhf = 0;
}