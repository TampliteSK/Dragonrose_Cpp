// init.cpp

#include "attackgen.hpp"
#include "zobrist.hpp"

void init_all() {

	// attackgen.hpp
	init_leapers_attacks();
	init_sliders_attacks(IS_BISHOP);
	init_sliders_attacks(IS_ROOK);

	// zobrist.hpp
	init_hash_keys();
}