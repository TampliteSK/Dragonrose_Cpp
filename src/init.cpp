// init.cpp

#include "attackgen.hpp"
#include "zobrist.hpp"
#include "search.hpp"
#include "datatypes.hpp"
#include "bitboard.hpp"

Bitboard file_masks[8];
Bitboard rank_masks[8];
Bitboard adjacent_files[8];

// Function prototypes
void init_file_rank_masks();

void init_all() {

	// attackgen.hpp
	init_leapers_attacks();
	init_sliders_attacks(IS_BISHOP);
	init_sliders_attacks(IS_ROOK);

	init_hash_keys(); // zobrist.hpp
	init_file_rank_masks(); // init.cpp
	init_LMR_table(); // search.hpp
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