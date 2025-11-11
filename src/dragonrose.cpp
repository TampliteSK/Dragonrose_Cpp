// dragonrose.cpp

#include <cstdint>
#include <cstdlib>  // For exit
#include <cstring>  // For strncmp
#include <iostream>
#include <string>

#include "UciHandler.hpp"
#include "chess/Board.hpp"
#include "chess/bench.hpp"
#include "eval/evaluate.hpp"
#include "init.hpp"
#include "search.hpp"
#include "timeman.hpp"
#include "ttable.hpp"

int main(int argc, char *argv[]) {
    init_all();

    // Use stack allocation with automatic cleanup (RAII)
    Board pos_storage;
    Board *pos = &pos_storage;
    reset_board(pos);

    SearchInfo info_storage;
    SearchInfo *info = &info_storage;
    init_searchinfo(info);

    HashTable hash_table_storage;  // Automatic cleanup via destructor
    HashTable *hash_table = &hash_table_storage;

    UciOptions options = UciOptions();
    UciHandler uci = UciHandler();

    // Handle CLI Arguments
    for (int arg_num = 0; arg_num < argc; ++arg_num) {
        if (strncmp(argv[arg_num], "bench", 5) == 0) {
            run_bench(pos, hash_table, info, uci);
            // No manual cleanup needed - RAII handles it
            return EXIT_SUCCESS;
        }
    }

    std::string line;
    while (true) {
        std::getline(std::cin, line);

        if (line.empty()) continue;

        if (line.substr(0, 3) == "uci") {
            uci.uci_loop(pos, hash_table, info, &options);
            if (info->quit == true) break;
            continue;
        } else if (line.substr(0, 4) == "test") {
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
        } else if (line.substr(0, 4) == "quit") {
            break;
        }
    }

    // No manual cleanup needed - RAII handles everything automatically
    return EXIT_SUCCESS;
}