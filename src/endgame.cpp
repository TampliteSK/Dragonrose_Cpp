// endgame.cpp

#include "datatypes.hpp"
#include "bitboard.hpp"
#include "endgame.hpp"
#include "evaluate.hpp"
#include <random>

// Returns an integer between -width ~ width cp as a draw score
// Prevents getting stuck at local minima in winning endgames, and pruning all lines in drawn endgames
int8_t endgame_noise(uint8_t width) {
	std::random_device rd; // Random number for seed
	std::mt19937_64 mt(rd()); // 64-bit Mersenne Twister
	std::uniform_int_distribution<uint64_t> distribution(-width, width);
	return distribution(mt);
}

// Determines if the position is a drawn endgame (exc. pawns)
bool check_material_draw(const Board* pos) {
	// Upper bound of a bishop's value in the endgame (slightly higher than actual value due to tapered eval)
	#define MAX_MINOR_PIECE 310

	return abs(count_material(pos)) < MAX_MINOR_PIECE;
}

/*
// Grants constant boosts for completely winning endgames
// Might lead to fluctuating / spiky evals
// Overkill (QQ, Q+R, RR): 20000
// Normal Win (Q, R): 10000
// Baseline (syzygy, B+N): 5000
uint16_t check_material_win(const Board* pos) {
	uint8_t wKnight = pos->piece_num[wN];
	uint8_t wBishop = pos->piece_num[wB];
	uint8_t wRook = pos->piece_num[wR];
	uint8_t wQueen = pos->piece_num[wQ];
	uint8_t bKnight = pos->piece_num[bN];
	uint8_t bBishop = pos->piece_num[bB];
	uint8_t bRook = pos->piece_num[bR];
	uint8_t bQueen = pos->piece_num[bQ];

	if (count_bits(pos->occupancy[BOTH]) <= 4) {

		// Overkill mates
		if (wQueen == 2 || (wQueen == 1 && wRook == 1) || wRook == 2) {
			return 20000;
		}
		else if (bQueen == 2 || (bQueen == 1 && bRook == 2) || bRook == 2) {
			return -20000;
		}

		// K+Q v K or K+R v K
		if (wQueen == 1 || wRook == 1) {
			return 10000;
		}
		else if (bQueen == 1 || bRook == 1) {
			return -10000;
		}

		// K+BN v K
		if (wBishop == 1 && wKnight == 1) {
			return 5000;
		}
		else if (bBishop == 1 && bKnight == 2) {
			return -5000;
		}
	}

	return 0;
}
*/