// ttable.cpp

#include <iostream>
#include <iomanip>
#include "ttable.hpp"
#include "Board.hpp"
#include "makemove.hpp"
#include "moveio.hpp"

std::string ascii_flags[] = { "None", "Alpha", "Beta", "Exact" };

// Function prototype
void print_hash_bucket(HashBucket bucket, int index);

int probe_PV_move(const Board* pos, const HashTable* table) {
	int index = pos->hash_key % table->max_entries;

	for (uint8_t i = 0; i < BUCKET_SIZE; ++i) {
		if (table->pTable[index].entries[i].hash_key == pos->hash_key) {
			std::cout << "Probed entry index " << index << " with depth " << (int)table->pTable[index].entries[i].depth << "\n";
			return table->pTable[index].entries[i].move;
		}
	}

	return NO_MOVE;
}

// Returns number of moves in the PV
int get_PV_line(Board* pos, const HashTable* table, const uint8_t depth) {
	int move = probe_PV_move(pos, table);
	int count = 0;

	// Try the moves of the stored PV to see if they are legal
	while (move != NO_MOVE && count < depth) {
		if (move_exists(pos, move)) {
			make_move(pos, move);
			pos->PV_array[count++] = move;
		}
		else {
			break;
		}
		move = probe_PV_move(pos, table);
	}

	while (pos->ply > 0) {
		take_move(pos);
	}

	return count;
}

void clear_hash_table(HashTable* table) {
	for (uint64_t i = 0; i < table->max_entries; ++i) {
		for (uint8_t j = 0; j < BUCKET_SIZE; ++j) {
			table->pTable[i].entries[j].hash_key = 0ULL;
			table->pTable[i].entries[j].move = NO_MOVE;
			table->pTable[i].entries[j].depth = 0;
			table->pTable[i].entries[j].score = 0;
			table->pTable[i].entries[j].flags = 0;
			table->pTable[i].entries[j].age = 0;
		}
	}
	table->num_entries = 0;
	table->new_write = 0;
	table->table_age = 0;
}

void init_hash_table(HashTable* table, const uint16_t MB) {

	int HashSize = 0x100000 * MB;
	table->max_entries = HashSize / sizeof(HashBucket) - 2;

	if (table->pTable != NULL) {
		free(table->pTable);
	}

	table->pTable = (HashBucket*)malloc(table->max_entries * sizeof(HashBucket));
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

	int index = pos->hash_key % table->max_entries;

	for (uint8_t i = 0; i < BUCKET_SIZE; ++i) {
		if (table->pTable[index].entries[i].hash_key == pos->hash_key) {
			move = table->pTable[index].entries[i].move;
			if (table->pTable[index].entries[i].depth >= depth) {
				table->hit++;

				score = table->pTable[index].entries[i].score;
				if (score > MATE_SCORE)		  score -= pos->ply;
				else if (score < -MATE_SCORE) score += pos->ply;

				switch (table->pTable[index].entries[i].flags) {
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
	}

	return false;
}

void store_hash_entry(Board* pos, HashTable* table, const int move, int score, const uint8_t flags, const uint8_t depth) {

	int index = pos->hash_key % table->max_entries;
	bool replace = false;

	// Check if there is an existing entry in the bucket
	uint8_t worst_entry = 0;
	uint32_t worst_age = 0;
	uint8_t worst_depth = 0;
	for (uint8_t i = 0; i < BUCKET_SIZE; ++i) {
		// Store entry if it doesn't exist
		if (table->pTable[index].entries[i].hash_key == 0) {
			table->new_write++;
			table->num_entries++;
			replace = true;
			break;
		}
		// Find the worst entry with the same hash key
		else if (table->pTable[index].entries[i].hash_key == pos->hash_key) {
			// print_hash_bucket(table->pTable[index], index);
			uint32_t entry_age = table->pTable[index].entries[i].age;
			uint8_t entry_depth = table->pTable[index].entries[i].depth;
			if (entry_age < worst_age || entry_depth < worst_depth) {
				worst_entry = i;
				worst_age = entry_age;
				worst_depth = entry_depth;
			}
		}
	}

	// Now replace the worst entry in the bucket
	if (table->table_age > worst_age || depth >= worst_depth) {
		replace = true;
		table->overwrite++;
	}

	if (!replace) return; // No need to overwrite the entry

	/*
	if (table->pTable[index].hash_key != 0 && depth >= 4) {
		std::cout << "Old key: " << std::hex << table->pTable[index].hash_key << " -> New key: " << std::hex << pos->hash_key
			<< " | Depth: " << (int)table->pTable[index].depth << " -> " << (int)depth
			<< " | Move: " << print_move(move) << ", Eval: " << std::dec << score
			<< " | Index: " << std::dec << index << "\n";
	}
	*/


	if (score > MATE_SCORE) score -= pos->ply;
	else if (score < -MATE_SCORE) score += pos->ply;

	table->pTable[index].entries[worst_entry].move = move;
	/*
	if (pos->hash_key != table->pTable[index].entry[worst_entry].hash_key) {
		std::cout << "hash collision at index " << index << "\n";
	}
	*/
	table->pTable[index].entries[worst_entry].hash_key = pos->hash_key;
	table->pTable[index].entries[worst_entry].flags = flags;
	table->pTable[index].entries[worst_entry].score = score;
	table->pTable[index].entries[worst_entry].depth = depth;
	table->pTable[index].entries[worst_entry].age = table->table_age;
}

/*
	Misc debug functions
*/

void print_hash_bucket(HashBucket bucket, int index) {
	std::cout << "Bucket (index " << index << ")\n";
	std::cout << "ID |    Hash Key    | Age | Depth | Move | Score | Flag\n";
	for (uint8_t i = 0; i < BUCKET_SIZE; ++i) {
		std::cout << std::setw(3) << (int)i << "|"
			<< std::setw(16) << std::hex << bucket.entries[i].hash_key << "|"
			<< std::setw(5) << std::dec << bucket.entries[i].age << "|"
			<< std::setw(7) << (int)bucket.entries[i].depth << "|"
			<< std::setw(6) << print_move(bucket.entries[i].move) << "|"
			<< std::setw(7) << bucket.entries[i].score << "|"
			<< std::setw(5) << ascii_flags[bucket.entries[i].flags] << "\n";
	}
}