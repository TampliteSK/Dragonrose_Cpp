// UciHandler.cpp

#include "Board.hpp"
#include "datatypes.hpp"
#include "makemove.hpp"
#include "moveio.hpp"
#include "search.hpp"
#include "timeman.hpp"
#include "UciHandler.hpp"
#include <cstdint>
#include <iostream>
#include <sstream>

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

void UciHandler::parse_go(Board* pos, HashTable* table, SearchInfo* info, const std::string& line) {
    int depth = -1, movestogo = 30, movetime = -1;
    int time = -1, inc = 0;
    info->timeset = false;

    if (line.find("infinite") != std::string::npos) {
        // Handle infinite case if necessary
    }

    if (pos->get_side() == BLACK) {
        inc = get_value_from_line(line, "binc");
    }
    if (pos->get_side() == WHITE) {
        inc = get_value_from_line(line, "winc");
    }
    if (pos->get_side() == WHITE) {
        time = get_value_from_line(line, "wtime");
    }
    if (pos->get_side() == BLACK) {
        time = get_value_from_line(line, "btime");
    }

    movestogo = get_value_from_line(line, "movestogo");
    movetime = get_value_from_line(line, "movetime");
    depth = get_value_from_line(line, "depth");

    if (movetime != -1) {
        time = movetime;
        movestogo = 1;
    }

    info->start_time = get_time_ms();
    info->depth = depth;

    // Time Management
    if (time != -1) {
        info->timeset = true;
        double time_allocated = time;
        uint8_t phase_moves = 0;

        if (time < 30000) { // 30s
            time_allocated /= 80;
        }
        else {
            if (pos->get_his_ply() <= 30) {
                time_allocated *= 0.1;
                phase_moves = round((30 - pos->get_his_ply() + ((pos->get_side() == BLACK) ? 0 : 1)) / 2.0);
                time_allocated /= phase_moves;
            }
            else if (pos->get_his_ply() <= 70) {
                time_allocated *= 0.45;
                phase_moves = round((70 - pos->get_his_ply() + ((pos->get_side() == BLACK) ? 0 : 1)) / 2.0);
                time_allocated /= phase_moves;
            }
            else {
                time_allocated /= 35;
            }
        }

        time_allocated -= 50; // overhead
        info->stop_time = int(info->start_time + time_allocated + inc / 2);
    }
    else {
        // No time was given. Just run until depth is reached
        info->stop_time = info->start_time + 3600000; // 1 hour window
    }

    if (depth == -1) {
        info->depth = MAX_DEPTH;
    }

    std::cout << "time: " << time << " start: " << info->start_time << " stop: " << info->stop_time << " depth: " << (int)info->depth << " timeset: " << info->timeset << "\n";
    search_position(pos, table, info);
}

void UciHandler::parse_position(Board* pos, const std::string& line) {
    std::string input = line.substr(9); // Skip "position "

    if (input.substr(0, 8) == "startpos") {
        pos->parse_fen(START_POS);
    }
    else {
        size_t fen_pos = input.find("fen");
        if (fen_pos == std::string::npos) {
            pos->parse_fen(START_POS);
        }
        else {
            pos->parse_fen(input.substr(fen_pos + 4)); // Skip "fen "
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
    // Disable buffering for stdin and stdout
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string line;
    std::cout << "id name " << ENGINE_NAME << std::endl;
    std::cout << "id author Tamplite Siphron Kents" << std::endl;

    std::cout << "option name Hash type spin default 16 min 4 max " << MAX_HASH << std::endl;
    int MB = 16;
    init_hash_table(table, MB);
    std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;
    std::cout << "option name Book type check default false" << std::endl;
    options->use_book = false;
    std::cout << "uciok" << std::endl;

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
            parse_position(pos, "position startpos\n");
        }
        else if (line.substr(0, 2) == "go") {
            parse_go(pos, table, info, line);
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
                MB = CLAMP(new_MB, 4, MAX_HASH);
                init_hash_table(table, MB);
                std::cout << "Set Hash to " << MB << " MB" << std::endl;
            }
            else {
                std::cout << "Invalid Hash value" << std::endl;
            }
        }
        else if (line.substr(0, 26) == "setoption name Book value ") {
            if (line.find("true") != std::string::npos) {
                std::cout << "Set Book to true" << std::endl;
                options->use_book = true;
            }
            else {
                std::cout << "Set Book to false" << std::endl;
                options->use_book = false;
            }
        }

        if (info->quit) break;
    }
}