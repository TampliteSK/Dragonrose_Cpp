// init.cpp

#include "chess/attackgen.hpp"
#include "chess/bitboard.hpp"
#include "chess/Board.hpp"
#include "chess/zobrist.hpp"

#include "datatypes.hpp"
#include "search.hpp"

Bitboard file_masks[8] = { 0ULL };
Bitboard rank_masks[8] = { 0ULL };
Bitboard adjacent_files[8] = { 0ULL };
Bitboard white_passer_masks[64] = { 0ULL };
Bitboard black_passer_masks[64] = { 0ULL };

// Function prototypes
void init_file_rank_masks();
void init_passer_masks();

void init_all() {

	// attackgen.hpp
	init_leapers_attacks();
	init_sliders_attacks(IS_BISHOP);
	init_sliders_attacks(IS_ROOK);

	init_hash_keys();       // zobrist.hpp
	init_file_rank_masks(); // init.cpp
	init_passer_masks();    // init.cpp
	init_LMR_table();       // search.hpp
}

void init_file_rank_masks() {
	for (int file = FILE_A; file <= FILE_H; ++file) {
		file_masks[file] = bits_between_squares(FR2SQ(file, RANK_1), FR2SQ(file, RANK_8));
	}
	for (int file = FILE_A; file <= FILE_H; ++file) {
		if (file == FILE_A) {
			adjacent_files[file] = file_masks[FILE_B];
		}
		else if (file == FILE_H) {
			adjacent_files[file] = file_masks[FILE_G];
		}
		else {
			adjacent_files[file] = file_masks[file - 1] | file_masks[file + 1];
		}
	}
	for (int rank = RANK_8; rank <= RANK_1; ++rank) {
		rank_masks[rank] = bits_between_squares(FR2SQ(FILE_A, rank), FR2SQ(FILE_H, rank));
	}
}

void init_passer_masks() {
	int16_t target_sq;

	for (uint8_t sq = 0; sq < 64; ++sq) {
		target_sq = sq + 8;
        while (target_sq < 64) {
            black_passer_masks[sq] |= (1ULL << target_sq);
            target_sq += 8;
        }

        target_sq = sq - 8;
        while (target_sq >= 0) {
            white_passer_masks[sq] |= (1ULL << target_sq);
            target_sq -= 8;
        }

        if (GET_FILE(sq) > FILE_A) {
            target_sq = sq + 7;
            while (target_sq < 64) {
                black_passer_masks[sq] |= (1ULL << target_sq);
                target_sq += 8;
            }

            target_sq = sq - 9;
            while (target_sq >= 0) {
                white_passer_masks[sq] |= (1ULL << target_sq);
                target_sq -= 8;
            }
        }

        if (GET_FILE(sq) < FILE_H) {
            target_sq = sq + 9;
            while (target_sq < 64) {
                black_passer_masks[sq] |= (1ULL << target_sq);
                target_sq += 8;
            }

            target_sq = sq - 7;
            while (target_sq >= 0) {
                white_passer_masks[sq] |= (1ULL << target_sq);
                target_sq -= 8;
            }
        }
	}
}