// ttable.hpp

#ifndef TTABLE_HPP
#define TTABLE_HPP

#include <cstdint>
#include "datatypes.hpp"
#include "Board.hpp"

class Board;

constexpr uint32_t MAX_HASH = 65536;

// Hash entry flags
enum { HFNONE, HFALPHA, HFBETA, HFEXACT };

// Hash entry struct
// 20 bytes (will be padded to 24)
typedef struct {
	uint64_t hash_key;
	int move;
	uint32_t score;
	uint8_t depth;
	uint8_t flags;
	uint16_t age; // indicates how new an entry is
} HashEntry;

// Hash table struct
typedef struct {
	HashEntry* pTable;
	uint32_t max_entries; // maximum entries based on given hash size
	uint32_t num_entries; // number of entries at any given time
	int new_write;
	int overwrite;
	int hit; // tracks the number of entires probed
	int cut; // max number of probes allowed before hash table is full (to avoid collision of entries)
	uint16_t table_age; // increments every move
} HashTable;

// Functions
int probe_PV_move(const Board* pos, const HashTable* table);
int get_PV_line(Board* pos, const HashTable* table, const uint8_t depth);
void clear_hash_table(HashTable* table);
void init_hash_table(HashTable* table, const uint16_t MB);
bool probe_hash_entry(Board* pos, HashTable* table, int& move, int& score, int alpha, int beta, int depth);
void store_hash_entry(Board* pos, HashTable* table, const int move, uint32_t score, const uint8_t flags, const uint8_t depth);

#endif // TTABLE_HPP