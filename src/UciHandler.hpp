// UciHandler.hpp

/*
	Forked from VICE uci.c by Richard Allbert (Bluefever Software)
*/

#ifndef UCIHANDLER_HPP
#define UCIHANDLER_HPP

#include "Board.hpp"
#include "search.hpp"

#define ENGINE_NAME "Dragonrose 0.29"

// UCI options struct
typedef struct {
	uint16_t hash_size; // type spin
	uint16_t threads; // type spin
	// bool use_book; // type check
} UciOptions;

class UciHandler {
public:
	UciHandler(); // Blank constructor

	void parse_go(Board* pos, HashTable* table, SearchInfo* info, const std::string& line);
	void parse_position(Board* pos, const std::string& line);
	void uci_loop(Board* pos, HashTable* table, SearchInfo* info, UciOptions* options);

private:
	int get_value_from_line(const std::string& line, const std::string& key);
};

#endif // UCIHANDLER_HPP