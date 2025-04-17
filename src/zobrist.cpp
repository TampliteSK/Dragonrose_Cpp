// zobrist.cpp

#include <cstdint>
#include <random>
#include "zobrist.hpp"
#include "Board.hpp"

// Init globals
uint64_t piece_keys[13][64] = { {0} }; // random piece keys [piece][square]
uint64_t castle_keys[16] = { 0 };      // random castling keys
uint64_t side_key = 0ULL;              // random side key, indicating white to move

uint64_t generate_random_U64() {
    std::random_device rd;    // Random number for seed
    std::mt19937_64 mt(rd()); // 64-bit Mersenne Twister
    std::uniform_int_distribution<uint64_t> distribution(0, UINT64_MAX);
    return distribution(mt);
}

void init_hash_keys() {
	for (int pce = 0; pce < 13; ++pce) {
		for (int sq = 0; sq < 64; ++sq) {
			piece_keys[pce][sq] = generate_random_U64();
		}
	}
	side_key = generate_random_U64();
	for (int index = 0; index < 16; ++index) {
		castle_keys[index] = generate_random_U64();
	}

}

uint64_t generate_hash_key(const Board *pos) {

	uint64_t final_key = 0ULL;

	// Pieces
	for (int sq = 0; sq < 64; ++sq) {
		uint8_t piece = pos->pieces[sq];
		if (piece != EMPTY) {
			final_key ^= piece_keys[piece][sq];
		}
	}

	if (pos->side == WHITE) {
		final_key ^= side_key;
	}

	if (pos->enpas != NO_SQ) {
		final_key ^= piece_keys[EMPTY][pos->enpas];
	}

	final_key ^= castle_keys[pos->castle_perms];

	return final_key;
}
