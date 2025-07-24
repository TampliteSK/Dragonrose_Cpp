// dragonrose.cpp

#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring> // For strncmp
#include <cstdlib> // For exit

#include "attack.hpp"
#include "Board.hpp"
#include "bitboard.hpp"
#include "dragonrose.hpp" // Bench positions
#include "evaluate.hpp"
#include "init.hpp"
#include "makemove.hpp"
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

        if (strncmp(argv[arg_num], "bench", 5) == 0) {

            init_hash_table(hash_table, 16);
            uint64_t total_nodes = 0;
            uint64_t start = get_time_ms();

            for (int index = 0; index < 50; ++index) {
                std::cout << "\n=== Benching position " << index+1 << "/50 ===\n";
                info->nodes = 0;
                std::cout << "Position: " << bench_positions[index] << "\n";
                parse_fen(pos, bench_positions[index]);
                uci->parse_go(pos, hash_table, info, "go depth 6");
                total_nodes += info->nodes;
            }

            uint64_t end = get_time_ms();
            uint64_t time = end - start;
            std::cout << "\n-#-#- Benchmark results -#-#-\n";
            std::cout << "Execution time: " << time / 1000.0 << "s \n";
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
        else if (line.substr(0, 4) == "test") {
            parse_fen(pos, "4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
            print_board(pos);
            int score = evaluate_pos(pos);
            std::cout << "Evaluation (white's perpsective): " << score / 100.0 << "\n\n";
            /*
            parse_fen(pos, "3rk2r/p7/1pb1p1pp/8/2P1P3/3BB1Pq/3Q1P2/3RR1K1 b k - 0 24");
            score = evaluate_pos(pos);
            std::cout << "Evaluation (black's perspective): " << score / 100.0 << "\n\n";
            */
            break;
        }
        else if (line.substr(0, 4) == "quit") {
            break;
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