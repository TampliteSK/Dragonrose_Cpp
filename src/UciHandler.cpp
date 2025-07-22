// UciHandler.cpp

#include "Board.hpp"
#include "datatypes.hpp"
#include "makemove.hpp"
#include "moveio.hpp"
#include "search.hpp"
#include "timeman.hpp"
#include "UciHandler.hpp"
#include "perft.hpp"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>

/*
    Private methods
*/

int UciHandler::get_value_from_line(const std::string& line, const std::string& key) {
    auto pos = line.find(key);
    if (pos != std::string::npos) {
        return std::atoi(line.substr(pos + key.length() + 1).c_str());
    }
    return -1;
}

/*
    Public methods
*/

// Constructor
UciHandler::UciHandler() {
    // Does absolutely nothing. No attributes to initialise.
}

// Handles go <> UCI commands
// Supports: go wtime <> btime <> winc <> binc <>
//           go movetime <>
//           go depth <>
//           go nodes <>
//           go infinite (although no stop command at the moment, can only be stopped via keyboard interrupt)
void UciHandler::parse_go(Board* pos, HashTable* table, SearchInfo* info, const std::string& line) {
    // int movestogo = 30;
    int time = -1, movetime = -1;
    int depth = -1, inc = 0, nodes = -1;
    info->timeset = false;
    info->nodesset = false;

    if (line.find("infinite") != std::string::npos) {
        // Nothing to handle
    }

    // Extract time control
    if (pos->side == BLACK) {
        inc = get_value_from_line(line, "binc");
    }
    if (pos->side == WHITE) {
        inc = get_value_from_line(line, "winc");
    }
    if (pos->side == WHITE) {
        time = get_value_from_line(line, "wtime");
    }
    if (pos->side == BLACK) {
        time = get_value_from_line(line, "btime");
    }

    // Extract other "go" options
    // movestogo is inaccurate and will not be supported in DR for the time being
    // movestogo = get_value_from_line(line, "movestogo"); 
    movetime = get_value_from_line(line, "movetime");
    depth = get_value_from_line(line, "depth");
    nodes = get_value_from_line(line, "nodes");

    if (movetime != -1) {
        time = movetime;
        // movestogo = 1;
    }

    info->start_time = get_time_ms();
    info->depth = depth;

    // Time Management
    if (time != -1) {
        // Add a buffer for handling Engine <-> GUI communication latency (esp. OpenBench - 250ms latency)
        info->timeset = true;
        constexpr int MIN_NETWORK_BUFFER = 250; // in ms

        // Get hard time limit
        int buffered_time = time + inc - std::min(time/2 + inc/2, MIN_NETWORK_BUFFER);
        double time_allocated = allocate_time(pos, buffered_time);
        info->hard_stop_time = uint64_t(info->start_time + time_allocated);

        // Get soft time limit
        buffered_time = time/20 + inc/2 - std::min(time/40 + inc/4, MIN_NETWORK_BUFFER);
        //     Prevent soft limit from exceeding hard limit
        info->soft_stop_time = std::min(info->start_time + buffered_time, info->hard_stop_time - MIN_NETWORK_BUFFER / 3);

        // std::cout << "Current time: " << get_time_ms() 
        //    << " | Hard limit: " << info->hard_stop_time << " (" << info->hard_stop_time - get_time_ms() << ") "
        //    << " | Soft limit: " << info->soft_stop_time << " (" << info->soft_stop_time - get_time_ms() << ") " << "\n";
    }

    if (depth == -1) {
        info->depth = MAX_DEPTH;
    }
    if (nodes != -1) {
        info->nodesset = true;
        info->nodes_limit = nodes;
    }

    // std::cout << "time: " << time << " start: " << info->start_time << " stop: " << info->stop_time << " depth: " << (int)info->depth << " timeset: " << info->timeset << "\n";
    // std::cout << "nodeset: " << info->nodesset << " | nodes limit: " << nodes << "\n";
    search_position(pos, table, info);
}

void UciHandler::parse_position(Board* pos, const std::string& line) {
    std::string input = line.substr(9); // Skip "position "

    if (input.substr(0, 8) == "startpos") {
        parse_fen(pos, START_POS);
    }
    else {
        size_t fen_pos = input.find("fen");
        if (fen_pos == std::string::npos) {
            parse_fen(pos, START_POS);
        }
        else {
            parse_fen(pos, input.substr(fen_pos + 4)); // Skip "fen "
        }
    }

    size_t moves_pos = input.find("moves");
    if (moves_pos != std::string::npos) {
        std::string movelist_str = input.substr(moves_pos + 6); // Skip "moves "

        while (!movelist_str.empty()) {
            size_t space_pos = movelist_str.find(' ');
            std::string move_str = (space_pos == std::string::npos) ? movelist_str : movelist_str.substr(0, space_pos);
            int move = parse_move(pos, move_str);
            if (move == NO_MOVE) break;
            make_move(pos, move);

            // Erase the processed move from movesPart
            movelist_str.erase(0, space_pos == std::string::npos ? movelist_str.length() : space_pos + 1);
        }
    }
}

void UciHandler::uci_loop(Board* pos, HashTable* table, SearchInfo* info, UciOptions* options) {

    std::string line;
    std::cout << "id name " << ENGINE_NAME << std::endl;
    std::cout << "id author Tamplite Siphron Kents" << std::endl;

    // UCI Options
    std::cout << "option name Hash type spin default 16 min 4 max " << MAX_HASH << std::endl;
    int MB = 16;
    options->hash_size = 16;
    init_hash_table(table, MB);
    std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;
    std::cout << "uciok" << std::endl;

    parse_fen(pos, START_POS);

    while (true) {
        std::getline(std::cin, line);

        if (line.empty()) continue;

        if (line.substr(0, 7) == "isready") {
            std::cout << "readyok" << std::endl;
            continue;
        }
        else if (line.substr(0, 8) == "position") {
            parse_position(pos, line);
        }
        else if (line.substr(0, 10) == "ucinewgame") {
            clear_hash_table(table);
            parse_fen(pos, START_POS);
        }
        else if (line.substr(0, 2) == "go") {
            if (line.substr(0, 8) == "go perft") {
                // Parse depth from "go perft X" command
                int depth = 0;
                size_t depth_pos = line.find("perft ");
                if (depth_pos != std::string::npos) {
                    try {
                        depth = std::stoi(line.substr(depth_pos + 6));
                    }
                    catch (...) {
                        depth = 5; // Fall back to default depth
                    }
                }
                // parse_fen(pos, CPW_POS6);
                run_perft(pos, depth, true);
            }
            else {
                // Normal go command
                parse_go(pos, table, info, line);
            }  
        }
        else if (line.substr(0, 3) == "run") {
            parse_go(pos, table, info, "go infinite");
        }
        else if (line.substr(0, 4) == "quit") {
            info->quit = true;
            break;
        }
        else if (line.substr(0, 3) == "uci") {
            std::cout << "id name " << ENGINE_NAME << std::endl;
            std::cout << "id author Tamplite Siphron Kents" << std::endl;
            std::cout << "uciok" << std::endl;
        }
        else if (line.substr(0, 26) == "setoption name Hash value ") {
            std::istringstream iss(line.substr(26)); // Extract the relevant substring
            int new_MB;
            if (iss >> new_MB) { // Attempt to read the integer
                MB = CLAMP(new_MB, 4, (int)MAX_HASH);
                options->hash_size = MB;
                init_hash_table(table, MB);
                std::cout << "info string Set Hash to " << MB << " MB" << std::endl;
            }
            else {
                std::cout << "Invalid Hash value" << std::endl;
            }
        }
        else if (line.substr(0, 5) == "print") {
            print_board(pos);
        }
        else if (line.substr(0, 9) == "load test") {
            parse_fen(pos, "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
        }

        if (info->quit) break;
    }
}