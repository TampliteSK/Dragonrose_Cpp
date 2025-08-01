// perft.cpp

#include "../timeman.hpp"
#include "../StaticVector.hpp"

#include "perft.hpp"
#include "Board.hpp"
#include "movegen.hpp"
#include "makemove.hpp"
#include "moveio.hpp"
#include "bitboard.hpp"
#include "attackgen.hpp"

#include <iostream>
#include <cstdint>

uint64_t run_perft(Board* pos, uint8_t depth, bool print_info) {

    if (depth == 0) {
        return 0;
    }

    uint64_t nodes = 0;
    uint64_t start = 0;

    MoveList move_list;
    generate_moves(pos, move_list, false);

    if (print_info) {
        std::cout << "\n     Performance test\n\n";
        start = get_time_ms();
    }

    for (int move_count = 0; move_count < (int)move_list.length; ++move_count) {

        // Skip illegal moves
        if (!make_move(pos, move_list.moves[move_count].move)) {
            continue;
        }

        uint64_t old_nodes = nodes;

        if (depth == 1) {
            nodes++;
        }
        else {
            nodes += run_perft(pos, depth - 1, false);
        }

        uint64_t new_nodes = nodes - old_nodes;

        take_move(pos);

        // Print move if root level
        if (print_info) {
            int move = move_list.moves[move_count].move;
            std::cout << print_move(move) << ": " << new_nodes << "\n";
        }
    }

    // Print results if root level
    if (print_info) {
        uint64_t time = get_time_ms() - start;
        std::cout << "\n    Depth: " << (int)depth << "\n"
            << "    Nodes: " << nodes << "\n"
            << "     Time: " << time << "ms (" << (double)time / 1000 << "s)\n"
            << "      NPS: " << int(nodes / (double)time * 1000) << "\n\n";
    }

    return nodes;
}