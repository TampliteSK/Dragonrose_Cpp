// dragonrose.cpp

#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring> // For strncmp
#include <cstdlib> // For exit

#include "Board.hpp"
#include "bitboard.hpp"
#include "dragonrose.hpp" // Bench positions
#include "evaluate.hpp"
#include "init.hpp"
#include "makemove.hpp"
#include "movegen.hpp"
#include "moveio.hpp"
#include "perft.hpp"
#include "search.hpp"
#include "timeman.hpp"
#include "ttable.hpp"
#include "UciHandler.hpp"

int main(int argc, char* argv[]) {
	init_all();

	Board* pos = new Board(START_POS);
	SearchInfo* info = (SearchInfo*)malloc(sizeof(SearchInfo));
	init_searchinfo(info);
	HashTable* hash_table = (HashTable*)malloc(sizeof(HashTable)); // Global hash
	hash_table->pTable = NULL;
    UciOptions* options = (UciOptions*)malloc(sizeof(UciOptions));
    UciHandler* uci = new UciHandler();

    // Handle CLI Arguments
    for (int arg_num = 0; arg_num < argc; ++arg_num) {

        // Doesn't work properly atm, will fix later
        if (strncmp(argv[arg_num], "bench", 5) == 0) {

            init_hash_table(hash_table, 16);
            uint64_t total_nodes = 0;
            uint64_t start = get_time_ms();

            for (int index = 0; index < 50; ++index) {
                std::cout << "\n=== Benching position " << index << "/49 ===\n";
                info->nodes = 0;
                std::cout << "Position: " << bench_positions[index] << "\n";
                pos->parse_fen(bench_positions[index]);
                uci->parse_go(pos, hash_table, info, "go depth 3");
                total_nodes += info->nodes;
            }

            uint64_t end = get_time_ms();
            uint64_t time = end - start;
            std::cout << "\n-#-#- Benchmark results -#-#-\n";
            std::cout << total_nodes << " nodes " << int(total_nodes / (double)time) << " nps\n";

            // Quit after benching is finished
            info->quit = true;
            return 0;
        }
    }

    std::string line;
    while (true) {
        std::getline(std::cin, line);

        if (line.empty()) continue;

        if (line.substr(0, 3) == "uci") {
            uci->uci_loop(pos, hash_table, info, options);
            if (info->quit == true) break;
            continue;
        }
        else if (line.substr(0, 4) == "perft") {
            run_perft(pos, 5, true);
        }
        else if (line.substr(0, 4) == "quit") {
            break;
        }
        else if (line.substr(0, 4) == "test") {
            std::cout << "No tests\n";
        }
    }

    delete pos;
	free(info);
    free(hash_table->pTable);
	free(hash_table);
    free(options);

	return 0;
}