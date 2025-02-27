// perft.cpp

#include <iostream>
#include <vector>
#include <cstdint>
#include "perft.hpp"
#include "Board.hpp"
#include "movegen.hpp"
#include "makemove.hpp"
#include "timeman.hpp"
#include "moveio.hpp"

// perft driver
static inline void perft_driver(Board *pos, int depth, uint64_t &nodes) {

    if (depth == 0) {
        nodes++;
        return;
    }

    std::vector<Move> list;
    generate_moves(pos, list);

    for (int count = 0; count < list.size(); ++count) {
        if (!make_move(pos, list.at(count).move)) {
            continue;
        }
        perft_driver(pos, depth - 1, nodes);
        take_move(pos);
    }
}

// perft test
void perft_test(Board *pos, int depth, uint64_t &nodes) {

    if (depth <= 0) {
        return;
    }

    std::cout << "\n     Performance test\n\n";

    std::vector<Move> move_list;
    generate_moves(pos, move_list);
    long start = get_time_ms();
    Board* original = pos;

    for (int move_count = 0; move_count < move_list.size(); ++move_count) {

        std::cout << "Board equality check: " << (original == pos) << std::endl;
        std::cout << "Checking move #" << move_count << std::endl;

        if (!make_move(pos, move_list.at(move_count).move)) {
            std::cout << "Invalid move: " << print_move(move_list[move_count].move) << std::endl;
            continue;
        }

        uint64_t cummulative_nodes = nodes;
        perft_driver(pos, depth - 1, nodes);
        uint64_t old_nodes = nodes - cummulative_nodes;
        take_move(pos);

        // print move
        int move = move_list.at(move_count).move;
        std::cout << "     move: " << ascii_squares[get_move_source(move)]
            << ascii_squares[get_move_target(move)]
            << (get_move_promoted(move) ? ascii_pieces[get_move_promoted(move)] : ' ')
            << "  nodes: " << old_nodes << "\n";
    }

    // print results
    std::cout << "\n    Depth: " << depth << "\n"
        << "    Nodes: " << nodes << "\n"
        << "     Time: " << (double)(get_time_ms() - start) << "s\n"
        << "      NPS: " << nodes / (double)(get_time_ms() - start) * 1000 << "\n\n";
}