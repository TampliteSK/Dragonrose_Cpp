// ttable.hpp

#ifndef TTABLE_HPP
#define TTABLE_HPP

#include <cstdint>

#include "../datatypes.hpp"
#include "Board.hpp"

const uint32_t MAX_HASH = 262144;
// const uint8_t BUCKET_SIZE = 4;

// Hash entry flags
enum { HFNONE, HFALPHA, HFBETA, HFEXACT };

// Hash entry struct
// 20 bytes (will be padded to 24)
struct HashEntry {
    uint64_t hash_key;
    int move;
    int score;
    uint8_t depth;
    uint8_t flags;
    uint32_t age;  // indicates how new an entry is

    HashEntry() : hash_key(0), move(0), score(0), depth(0), flags(0), age(0) {}
};

/*
typedef struct {
        HashEntry entries[BUCKET_SIZE];
} HashBucket;
*/

// Hash table struct with RAII
struct HashTable {
    HashEntry *pTable;
    uint64_t max_entries;  // maximum entries based on given hash size
    uint64_t num_entries;  // number of entries at any given time
    int new_write;
    int overwrite;
    int hit;  // tracks the number of entires probed
    int cut;  // max number of probes allowed before hash table is full (to avoid collision of
              // entries)
    uint32_t table_age;  // increments every move

    // Constructor
    HashTable() : pTable(nullptr), max_entries(0), num_entries(0),
                  new_write(0), overwrite(0), hit(0), cut(0), table_age(0) {}

    // Destructor
    ~HashTable() {
        if (pTable != nullptr) {
            delete[] pTable;
            pTable = nullptr;
        }
    }

    // Disable copy (Rule of Five)
    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    // Enable move
    HashTable(HashTable&& other) noexcept
        : pTable(other.pTable), max_entries(other.max_entries),
          num_entries(other.num_entries), new_write(other.new_write),
          overwrite(other.overwrite), hit(other.hit), cut(other.cut),
          table_age(other.table_age) {
        other.pTable = nullptr;
        other.max_entries = 0;
    }

    HashTable& operator=(HashTable&& other) noexcept {
        if (this != &other) {
            if (pTable != nullptr) {
                delete[] pTable;
            }
            pTable = other.pTable;
            max_entries = other.max_entries;
            num_entries = other.num_entries;
            new_write = other.new_write;
            overwrite = other.overwrite;
            hit = other.hit;
            cut = other.cut;
            table_age = other.table_age;
            other.pTable = nullptr;
            other.max_entries = 0;
        }
        return *this;
    }
};

// Functions
int probe_PV_move(const Board *pos, const HashTable *table);
void get_PV_line(Board *pos, const HashTable *table, const uint8_t depth);
void clear_hash_table(HashTable *table);
void init_hash_table(HashTable *table, const uint16_t MB);
bool probe_hash_entry(Board *pos, HashTable *table, int &move, int &score, int alpha, int beta,
                      int &entry_depth, int depth);
void store_hash_entry(Board *pos, HashTable *table, const int move, int score, const uint8_t flags,
                      const uint8_t depth);

#endif  // TTABLE_HPP