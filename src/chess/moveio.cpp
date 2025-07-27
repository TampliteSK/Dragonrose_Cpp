// moveio.cpp

#include "../datatypes.hpp"
#include "moveio.hpp"
#include "movegen.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

// print move (for UCI purposes)
std::string print_move(int move) {
    std::ostringstream oss;
    oss << ascii_squares[get_move_source(move)]
        << ascii_squares[get_move_target(move)];

    // Promoted pieces must be encoded in lowercase
    if (get_move_promoted(move)) {
        int promoted_piece = get_move_promoted(move);
        if (piece_type[promoted_piece] == QUEEN) {
            oss << "q";
        }
        else if (piece_type[promoted_piece] == ROOK) {
            oss << "r";
        }
        else if (piece_type[promoted_piece] == BISHOP) {
            oss << "b";
        }
        else if (piece_type[promoted_piece] == KNIGHT) {
            oss << "n";
        }
    }

    return oss.str();
}

// Parses user/GUI move string input (e.g. "e7e8q") and checks if its valid.
// Returns the move if valid.
int parse_move(const Board *pos, std::string move_string) {

    StaticVector<Move, MAX_LEGAL_MOVES> move_list;
    generate_moves(pos, move_list, false);

    // Parse squares
    int source_square = (move_string[0] - 'a') + (8 - (move_string[1] - '0')) * 8;
    int target_square = (move_string[2] - 'a') + (8 - (move_string[3] - '0')) * 8;

    // Search if the move exists in the list
    for (int move_count = 0; move_count < (int)move_list.size(); move_count++) {

        int move = move_list[move_count].move;

        // make sure source & target squares are available within the generated move
        if (source_square == get_move_source(move) && target_square == get_move_target(move)) {

            int promoted_piece = get_move_promoted(move);

            if (promoted_piece) {
                if (piece_type[promoted_piece] == QUEEN && move_string[4] == 'q') {
                    return move;
                }
                else if (piece_type[promoted_piece] == ROOK && move_string[4] == 'r') {
                    return move;
                }
                else if (piece_type[promoted_piece] == BISHOP && move_string[4] == 'b') {
                    return move;
                }
                else if (piece_type[promoted_piece] == KNIGHT && move_string[4] == 'n') {
                    return move;
                }
                continue; // continue the loop on possible wrong promotions (e.g. "e7e8f")
            }

            return move;
        }
    }

    return 0; // Illegal move
}

// print move list
void print_move_list(const StaticVector<Move, MAX_LEGAL_MOVES> move_list, bool verbose = true) {

    // Do nothing on empty move list
    if (move_list.empty()) {
        std::cout << "\n     No move in the move list!\n";
        return;
    }

    if (verbose) {
        std::cout << "\n     move    piece     capture   double    enpass    castling    score\n\n";

            // Loop over moves within a move list
            for (int move_count = 0; move_count < (int)move_list.size(); move_count++) {

                Move move_struct = move_list[move_count];
                int move = move_struct.move;
                int score = move_struct.score;

                // Print move with ASCII representation
                std::cout << "      " << ascii_squares[get_move_source(move)]
                    << ascii_squares[get_move_target(move)]
                    << (get_move_promoted(move) ? ascii_pieces[get_move_promoted(move)] : ' ')
                    << "   " << ascii_pieces[get_move_piece(move)]
                    << "         " << get_move_captured(move)
                    << "         " << (get_move_double(move) ? 1 : 0)
                    << "         " << (get_move_enpassant(move) ? 1 : 0)
                    << "         " << (get_move_castling(move) ? 1 : 0)
                    << "         " << score << "\n";
            }
    }
    else {
         std::cout << "Generated moves: ";

        for (int move_count = 0; move_count < (int)move_list.size(); move_count++) {
            Move move_struct = move_list[move_count];
            int move = move_struct.move;
            int score = move_struct.score;
            std::cout << print_move(move) << " (" << score << ") ";
        }
    }

    // Print total number of moves
    std::cout << "\n\n     Total number of moves: " << move_list.size() << "\n\n";
}