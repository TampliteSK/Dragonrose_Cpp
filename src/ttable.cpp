// ttable.cpp

#include <iostream>
#include "ttable.hpp"
#include "Board.hpp"
#include "makemove.hpp"
#include "moveio.hpp"

HashTable table[1];

int probe_PV_move(const Board* pos, const HashTable* table) {
	int index = pos->get_hash_key() % table->max_entries;

	if (table->pTable[index].hash_key == pos->get_hash_key()) {
		return table->pTable[index].move;
	}

	return NO_MOVE;
}

// Returns number of moves in the PV
int get_PV_line(Board* pos, const HashTable* table, const uint8_t depth) {

	Board* copy = pos->clone();

	int move = probe_PV_move(copy, table);
	int count = 0;

	// Try the moves of the stored PV to see if they are legal
	while (move != NO_MOVE && count < depth) {
		if (!make_move(copy, move)) {
			break; // Illegal move
		}
		else {
			pos->set_PV_move(count, move);
			count++;
		}
		move = probe_PV_move(copy, table);
	}

	delete copy; // Revert to pos
	return count;
}

void clear_hash_table(HashTable* table) {
	for (int i = 0; i < table->max_entries; ++i) {
		table->pTable[i].hash_key = 0ULL;
		table->pTable[i].move = NO_MOVE;
		table->pTable[i].depth = 0;
		table->pTable[i].score = 0;
		table->pTable[i].flags = 0;
		table->pTable[i].age = 0;
	}

	table->num_entries = 0;
	table->new_write = 0;
	table->table_age = 0;
}

void init_hash_table(HashTable* table, const uint16_t MB) {

	int HashSize = 0x100000 * MB;
	table->max_entries = HashSize / sizeof(HashEntry) - 2;

	if (table->pTable != NULL) {
		free(table->pTable);
	}

	table->pTable = (HashEntry*)malloc(table->max_entries * sizeof(HashEntry));
	if (table->pTable == NULL) {
		std::cout << "malloc failed for " << MB << "MB. retrying\n";
		init_hash_table(table, MB / 2); // Retry hash allocation with half size if failed
	}
	else {
		clear_hash_table(table);
		// std::cout << "HashTable init complete with " << table->max_entries << " entries\n";
	}

}

bool probe_hash_entry(Board* pos, HashTable* table, int& move, int& score, int alpha, int beta, int depth) {

	int index = pos->get_hash_key() % table->max_entries;

	if (table->pTable[index].hash_key == pos->get_hash_key()) {
		move = table->pTable[index].move;
		if (table->pTable[index].depth >= depth) {
			table->hit++;

			score = table->pTable[index].score;
			if (score > MATE_SCORE)		  score -= pos->get_ply();
			else if (score < -MATE_SCORE) score += pos->get_ply();

			switch (table->pTable[index].flags) {
			case HFALPHA: if (score <= alpha) {
				score = alpha;
				return true;
			}
						break;
			case HFBETA: if (score >= beta) {
				score = beta;
				return true;
			}
					   break;
			case HFEXACT:
				return true;
				break;
			}
		}
	}

	return false;
}

void store_hash_entry(Board* pos, HashTable* table, const int move, uint32_t score, const uint8_t flags, const uint8_t depth) {

	int index = pos->get_hash_key() % table->max_entries;
	bool replace = false;

	// Check if there is an existing entry at a certain index
	if (table->pTable[index].hash_key == 0) {
		// Store entry if it doesn't exist
		table->new_write++;
		table->num_entries++;
		replace = true;
	}
	else {
		int entry_age = table->pTable[index].age;
		uint8_t entry_depth = table->pTable[index].depth;
		
		// Replacement condition 1: Greater age (newer entry)
		if (table->table_age > entry_age) {
			replace = true;
			table->overwrite++;
		}
		// Replacement condition 2: Same age and greater depth
		else if (entry_age == table->table_age && depth > entry_depth) {
			replace = true;
			table->overwrite++;
		}
	}

	if (!replace) return; // No need to overwrite the entry

	int written_score = score;
	if (score > MATE_SCORE) {
		written_score += pos->get_ply();
	}
	else if (score < -MATE_SCORE) {
		written_score -= pos->get_ply();
	}

	table->pTable[index].move = move;
	table->pTable[index].hash_key = pos->get_hash_key();
	table->pTable[index].flags = flags;
	table->pTable[index].score = written_score;
	table->pTable[index].depth = depth;
	table->pTable[index].age = table->table_age;
}