// search.hpp

#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <cstdint>

#include "../chess/Board.hpp"
#include "StaticVector.hpp"
#include "datatypes.hpp"
#include "ttable.hpp"

typedef struct {
    bool timeset;   // go with time
    bool nodesset;  // go nodes <>

    uint64_t start_time;
    uint64_t soft_stop_time;  // Soft time limit
    uint64_t hard_stop_time;  // Hard time limit

    uint8_t depth;
    uint8_t seldepth;
    uint64_t nodes;
    uint64_t nodes_limit;

    uint16_t movestogo;
    bool quit;
    bool stopped;
    bool soft_stopped;

    float fh;   // beta cutoffs
    float fhf;  // legal moves
} SearchInfo;

extern int LMR_reduction_table[MAX_DEPTH][MAX_PSEUDO_MOVES][2];  // [ply][move_num][is_quiet]

// Functions
void search_position(Board& pos, HashTable& table, SearchInfo& info);
void init_searchinfo(SearchInfo& info);
void init_LMR_table();

#endif  // SEARCH_HPP