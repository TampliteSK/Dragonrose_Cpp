// zobrist.hpp

#ifndef ZOBRIST_HPP
#define ZOBRIST_HPP

#include "Board.hpp"

extern uint64_t piece_keys[13][64];  // random piece keys [piece][square]
extern uint64_t castle_keys[16];     // random castling keys
extern uint64_t side_key;            // random side key

// Functions
void init_hash_keys();
uint64_t generate_hash_key(const Board *pos);

#endif  // ZOBRIST_HPP