// dragonrose.cpp

#include <cstdint>
#include <cstdlib>  // For exit
#include <cstring>  // For strncmp
#include <iostream>
#include <memory>
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

    auto pos = std::make_unique<Board>();
    reset_board(*pos);
    auto info = std::make_unique<SearchInfo>();
    init_searchinfo(*info);
    auto hash_table = std::make_unique<HashTable>();
    hash_table->pTable = nullptr;
    UciOptions options = UciOptions();
    UciHandler uci = UciHandler();

    // Handle CLI Arguments
    for (int arg_num = 0; arg_num < argc; ++arg_num) {
        if (strncmp(argv[arg_num], "bench", 5) == 0) {
            run_bench(*pos, *hash_table, *info, uci);
            return EXIT_SUCCESS;
        }
    }

    // Enter UCI loop immediately
    uci.uci_loop(*pos, *hash_table, *info, &options);

    return EXIT_SUCCESS;
}