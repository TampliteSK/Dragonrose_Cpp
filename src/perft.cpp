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
#include "bitboard.hpp"
#include "attackgen.hpp"

uint64_t run_perft(Board* pos, uint8_t depth, bool print_info) {

    if (depth == 0) {
        return 0;
    }

    uint64_t nodes = 0;
    uint64_t start = 0;

    std::vector<Move> move_list;
    generate_moves(pos, move_list, false);

    if (print_info) {
        print_move_list_compact(move_list);
    }

    if (print_info) {
        std::cout << "\n     Performance test\n\n";
        start = get_time_ms();
    }

    for (int move_count = 0; move_count < move_list.size(); ++move_count) {

        Board* copy = pos->clone();

        if (depth == 2) {
            pos->print_board();
        }

        // Skip illegal moves
        if (!make_move(pos, move_list.at(move_count).move)) {
            std::cout << "Illegal move: " << print_move(move_list.at(move_count).move) << "\n";
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

        bool equal_pos = check_boards(pos, copy);

        delete copy;

        // Print move if root level
        if (print_info) {
            int move = move_list.at(move_count).move;
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

/*
// Run perft (performance test). Returns total number of nodes
uint64_t run_perft(Board *pos, uint8_t depth, bool print_info) {

    if (depth == 0) {
        return 0;
    }

    uint64_t nodes = 0;
    uint64_t start = 0;

    std::vector<Move> move_list;
    generate_moves(pos, move_list, false);

    if (print_info) {
        std::cout << "\n     Performance test\n\n";
        start = get_time_ms();
    }

    for (int move_count = 0; move_count < move_list.size(); ++move_count) {
        Board* copy = pos->clone(); // Start with a fresh copy every move
        
        // Skip illegal moves
        if (!make_move(copy, move_list.at(move_count).move)) {
            delete copy;
            continue;
        }

        uint64_t old_nodes = nodes;

        if (depth == 1) {
            nodes++;
        }
        else {
            nodes += run_perft(copy, depth - 1, false);
        }
        
        uint64_t new_nodes = nodes - old_nodes;

        delete copy; // Essentially "take move“

        // Print move if root level
        if (print_info) {
            int move = move_list.at(move_count).move;
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
*/