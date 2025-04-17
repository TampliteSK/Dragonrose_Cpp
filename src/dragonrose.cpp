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
#include "search.hpp"
#include "timeman.hpp"
#include "ttable.hpp"
#include "UciHandler.hpp"

int main(int argc, char* argv[]) {
	init_all();

    Board* pos = (Board*)malloc(sizeof(Board));;
    reset_board(pos);
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
                std::cout << "\n=== Benching position " << index+1 << "/50 ===\n";
                info->nodes = 0;
                std::cout << "Position: " << bench_positions[index] << "\n";
                parse_fen(pos, bench_positions[index]);
                clear_hash_table(hash_table);
                uci->parse_go(pos, hash_table, info, "go depth 4");
                total_nodes += info->nodes;
            }

            uint64_t end = get_time_ms();
            uint64_t time = end - start;
            std::cout << "\n-#-#- Benchmark results -#-#-\n";
            std::cout << total_nodes << " nodes " << int(total_nodes / (double)time * 1000) << " nps\n";

            // Quit after benching is finished
            free(pos);
            free(info);
            free(hash_table->pTable);
            free(hash_table);
            free(options);
            delete uci;
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
        else if (line.substr(0, 4) == "quit") {
            break;
        }
        else if (line.substr(0, 4) == "test") {
            parse_fen(pos, "QQQQQQBk/Q6B/Q6Q/Q6Q/Q6Q/Q6Q/Q6Q/KQQQQQQQ w - - 0 1");
            uint64_t start = get_time_ms();
            for (int i = 0; i < 100000; ++i) {
                std::vector<Move> list;
                generate_moves(pos, list, false);
            }
            uint64_t end = get_time_ms();
            std::cout << "Movegen time: " << end - start << "ms\n";
        }
    }

    free(pos);
	free(info);
    free(hash_table->pTable);
	free(hash_table);
    free(options);
    delete uci;

	return 0;
}