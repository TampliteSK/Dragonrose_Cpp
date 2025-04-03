// init.cpp

#include "attackgen.hpp"
#include "zobrist.hpp"
#include "search.hpp"

void init_all() {

	// attackgen.hpp
	init_leapers_attacks();
	init_sliders_attacks(IS_BISHOP);
	init_sliders_attacks(IS_ROOK);

	init_hash_keys(); // zobrist.hpp
	init_LMR_table(); // search.hpp
}