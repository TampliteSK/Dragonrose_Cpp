// search.hpp

#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <cstdint>

constexpr uint8_t MAX_DEPTH = 64;
constexpr uint16_t INF_BOUND = 30000;
constexpr uint16_t MATE_SCORE = INF_BOUND - MAX_DEPTH;

typedef struct {
	uint64_t start_time;
	uint64_t stop_time;
	uint8_t depth;
	uint64_t nodes;

	bool timeset;
	uint16_t movestogo;

	bool quit;
	bool stopped;

	float fh; // beta cutoffs
	float fhf; // legal moves
	uint16_t nullCut;

} SearchInfo;


#endif // SEARCH_HPP