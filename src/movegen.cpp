// movegen.cpp

#include <vector>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include "movegen.hpp"
#include "bitboard.hpp"
#include "Board.hpp"
#include "attack.hpp"
#include "attackgen.hpp"

/*
    Move generation
*/

// add move to the move list
static inline void add_move(std::vector<Move>& move_list, int move, int score) {
    Move move_struct = { move, score };
    move_list.push_back(move_struct);
}

// generate all moves
void generate_moves(const Board *pos, std::vector<Move>& move_list, bool noisy_only) {

    uint8_t source_square, target_square;
    Bitboard bitboard, attacks; // define current piece's bitboard copy & it's attacks

    for (int piece = wP; piece <= bK; piece++) {
        bitboard = pos->get_bitboard(piece);

        /*
            White pawn and castling moves
        */

        if (pos->get_side() == WHITE) {

            // White pawns
            if (piece == wP) {
                // loop over white pawns within white pawn bitboard
                while (bitboard) {
                    source_square = pop_ls1b(bitboard);
                    target_square = source_square - 8;

                    // Generate quiet pawn moves
                    if (!noisy_only) {
                        if (!(target_square < a8) && !GET_BIT(pos->get_occupancy(BOTH), target_square)) {

                            // pawn promotion
                            if (source_square >= a7 && source_square <= h7) {
                                add_move(move_list, encode_move(source_square, target_square, piece, wQ, 0, 0, 0, 0), 0);
                                add_move(move_list, encode_move(source_square, target_square, piece, wR, 0, 0, 0, 0), 0);
                                add_move(move_list, encode_move(source_square, target_square, piece, wB, 0, 0, 0, 0), 0);
                                add_move(move_list, encode_move(source_square, target_square, piece, wN, 0, 0, 0, 0), 0);
                            }
                            else {
                                // one square ahead pawn move
                                add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0), 0);

                                // two squares ahead pawn move
                                if ((source_square >= a2 && source_square <= h2) && !GET_BIT(pos->get_occupancy(BOTH), target_square - 8)) {
                                    add_move(move_list, encode_move(source_square, target_square - 8, piece, 0, 0, 1, 0, 0), 0);
                                }
                            }
                        }
                    }

                    attacks = pawn_attacks[pos->get_side()][source_square] & pos->get_occupancy(BLACK);

                    // generate pawn captures
                    while (attacks) {
                        // init target square
                        target_square = pop_ls1b(attacks);

                        // pawn promotion
                        if (source_square >= a7 && source_square <= h7) {
                            add_move(move_list, encode_move(source_square, target_square, piece, wQ, 1, 0, 0, 0), 0);
                            add_move(move_list, encode_move(source_square, target_square, piece, wR, 1, 0, 0, 0), 0);
                            add_move(move_list, encode_move(source_square, target_square, piece, wB, 1, 0, 0, 0), 0);
                            add_move(move_list, encode_move(source_square, target_square, piece, wN, 1, 0, 0, 0), 0);
                        }
                        else {
                            // Normal capture
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0), 0);
                        }
                    }

                    // generate enpassant captures
                    if (pos->get_enpas() != NO_SQ) {
                        Bitboard enpassant_attacks = pawn_attacks[pos->get_side()][source_square] & (1ULL << pos->get_enpas()); // Check if enpassant is a valid capture
                        if (enpassant_attacks) {
                            int target_enpassant = pop_ls1b(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0), 0);
                        }
                    }
                }
            }

            // castling moves
            if (!noisy_only) {
                if (piece == wK) {
                    // King side castling
                    if (pos->get_castle_perms() & WKCA) {
                        if (pos->get_piece(f1) == EMPTY && pos->get_piece(g1) == EMPTY) {
                            if (!is_square_attacked(pos, e1, BLACK) && !is_square_attacked(pos, f1, BLACK)) {
                                add_move(move_list, encode_move(e1, g1, piece, 0, 0, 0, 0, 1), 0);
                            }
                        }
                    }
                    // Queen side castling
                    if (pos->get_castle_perms() & WQCA) {
                        if (pos->get_piece(d1) == EMPTY && pos->get_piece(c1) == EMPTY && pos->get_piece(b1) == EMPTY) {
                            if (!is_square_attacked(pos, e1, BLACK) && !is_square_attacked(pos, d1, BLACK)) {
                                add_move(move_list, encode_move(e1, c1, piece, 0, 0, 0, 0, 1), 0);
                            }
                        }
                    }
                }
            }
        }

        /*
            Black pawns and castling moves
        */

        else {
            if (piece == bP) {
                while (bitboard) {
                    source_square = pop_ls1b(bitboard);
                    target_square = source_square + 8;

                    // Generate quiet pawn moves
                    if (!noisy_only) {
                        if (!(target_square > h1) && !GET_BIT(pos->get_occupancy(BOTH), target_square)) {

                            // pawn promotion
                            if (source_square >= a2 && source_square <= h2) {
                                add_move(move_list, encode_move(source_square, target_square, piece, bQ, 0, 0, 0, 0), 0);
                                add_move(move_list, encode_move(source_square, target_square, piece, bR, 0, 0, 0, 0), 0);
                                add_move(move_list, encode_move(source_square, target_square, piece, bB, 0, 0, 0, 0), 0);
                                add_move(move_list, encode_move(source_square, target_square, piece, bN, 0, 0, 0, 0), 0);
                            }
                            else {
                                // one square ahead pawn move
                                add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0), 0);

                                // two squares ahead pawn move
                                if ((source_square >= a7 && source_square <= h7) && !GET_BIT(pos->get_occupancy(BOTH), target_square + 8)) {
                                    add_move(move_list, encode_move(source_square, target_square + 8, piece, 0, 0, 1, 0, 0), 0);
                                }

                            }
                        }
                    }

                    attacks = pawn_attacks[pos->get_side()][source_square] & pos->get_occupancy(WHITE);

                    // generate pawn captures
                    while (attacks) {

                        target_square = pop_ls1b(attacks);

                        // pawn promotion
                        if (source_square >= a2 && source_square <= h2) {
                            add_move(move_list, encode_move(source_square, target_square, piece, bQ, 1, 0, 0, 0), 0);
                            add_move(move_list, encode_move(source_square, target_square, piece, bR, 1, 0, 0, 0), 0);
                            add_move(move_list, encode_move(source_square, target_square, piece, bB, 1, 0, 0, 0), 0);
                            add_move(move_list, encode_move(source_square, target_square, piece, bN, 1, 0, 0, 0), 0);
                        }
                        else {
                            // Normal capture
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0), 0);
                        }
                    }

                    // generate enpassant captures
                    if (pos->get_enpas() != NO_SQ) {
                        Bitboard enpassant_attacks = pawn_attacks[pos->get_side()][source_square] & (1ULL << pos->get_enpas());
                        if (enpassant_attacks) {
                            // init enpassant capture target square
                            int target_enpassant = pop_ls1b(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0), 0);
                        }
                    }
                }
            }

            // castling moves
            if (!noisy_only) {
                if (piece == bK) {
                    // King side castling
                    if (pos->get_castle_perms() & BKCA) {
                        if (pos->get_piece(f8) == EMPTY && pos->get_piece(g8) == EMPTY) {
                            if (!is_square_attacked(pos, e8, WHITE) && !is_square_attacked(pos, f8, WHITE)) {
                                add_move(move_list, encode_move(e8, g8, piece, 0, 0, 0, 0, 1), 0);
                            }
                        }
                    }
                    // Queen side castling
                    if (pos->get_castle_perms() & BQCA) {
                        if (pos->get_piece(d8) == EMPTY && pos->get_piece(c8) == EMPTY && pos->get_piece(b8) == EMPTY) {
                            if (!is_square_attacked(pos, e8, WHITE) && !is_square_attacked(pos, d8, WHITE)) {
                                add_move(move_list, encode_move(e8, c8, piece, 0, 0, 0, 0, 1), 0);
                            }
                        }
                    }
                }
            }
        }

        /*
            Knight moves
        */

        if (piece_type[piece] == KNIGHT && piece_col[piece] == pos->get_side()) {
            while (bitboard) {
                source_square = pop_ls1b(bitboard);
                // Remove attacks which land on friendly pieces
                attacks = knight_attacks[source_square] & ((pos->get_side() == WHITE) ? ~pos->get_occupancy(WHITE) : ~pos->get_occupancy(BLACK));

                while (attacks) {
                    target_square = pop_ls1b(attacks);

                    // Capture move 
                    // Check if the square is occupied by an enemy piece
                    if (piece_col[pos->get_piece(target_square)] == (pos->get_side() ^ 1)) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0), 0);
                    }
                    // Quiet move
                    else if (!noisy_only) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0), 0);
                    }
                }
            }
        }

        /*
            Bishop moves
        */

        if (piece_type[piece] == BISHOP && piece_col[piece] == pos->get_side()) {
            while (bitboard) {
                source_square = pop_ls1b(bitboard);
                attacks = get_bishop_attacks(source_square, pos->get_occupancy(BOTH)) & ((pos->get_side() == WHITE) ? ~pos->get_occupancy(WHITE) : ~pos->get_occupancy(BLACK));

                while (attacks) {
                    target_square = pop_ls1b(attacks);

                    // Capture move
                    if (piece_col[pos->get_piece(target_square)] == (pos->get_side() ^ 1)) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0), 0);
                    } 
                    // Quiet move
                    else if (!noisy_only) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0), 0);
                    }
                }
            }
        }

        /*
            Rook moves
        */

        if (piece_type[piece] == ROOK && piece_col[piece] == pos->get_side()) {
            while (bitboard) {
                source_square = pop_ls1b(bitboard);
                attacks = get_rook_attacks(source_square, pos->get_occupancy(BOTH)) & ((pos->get_side() == WHITE) ? ~pos->get_occupancy(WHITE) : ~pos->get_occupancy(BLACK));

                while (attacks) {
                    target_square = pop_ls1b(attacks);

                    // Capture move
                    if (piece_col[pos->get_piece(target_square)] == (pos->get_side() ^ 1)) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0), 0);
                    }
                    // Quiet move
                    else if (!noisy_only) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0), 0);
                    }
                }
            }
        }

        /*
            Queen moves
        */

        if (piece_type[piece] == QUEEN && piece_col[piece] == pos->get_side()) {
            while (bitboard) {
                source_square = pop_ls1b(bitboard);
                attacks = get_queen_attacks(source_square, pos->get_occupancy(BOTH)) & ((pos->get_side() == WHITE) ? ~pos->get_occupancy(WHITE) : ~pos->get_occupancy(BLACK));

                while (attacks) {
                    target_square = pop_ls1b(attacks);

                    // Capture move
                    if (piece_col[pos->get_piece(target_square)] == (pos->get_side() ^ 1)) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0), 0);
                    }
                    // Quiet move
                    else if (!noisy_only) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0), 0);
                    }
                }
            }
        }

        /*
            King moves (exc. castling)
        */

        if (piece_type[piece] == KING && piece_col[piece] == pos->get_side()) {
            while (bitboard) {
                source_square = pop_ls1b(bitboard);
                attacks = king_attacks[source_square] & ((pos->get_side() == WHITE) ? ~pos->get_occupancy(WHITE) : ~pos->get_occupancy(BLACK));

                while (attacks) {
                    target_square = pop_ls1b(attacks);

                    // Capture move
                    if (piece_col[pos->get_piece(target_square)] == (pos->get_side() ^ 1)) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0), 0);
                    }
                    // Quiet move
                    else if (!noisy_only) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0), 0);
                    }
                }
            }
        }
    }
}

/*
    Move scoring

    =======================
         Move ordering
    =======================

    1. PV move
    2. Captures in MVV/LVA
    3. 1st killer move
    4. 2nd killer move
    5. History moves
    6. Unsorted moves
*/

// score moves
static inline int score_move(const Board *pos, int move) {

    // Score PV moves
    // ...

    // Score capture move
    if (get_move_capture(move)) {
        int target_piece = pos->get_piece(get_move_target(move));
        return mvv_lva[get_move_piece(move) - 1][target_piece - 1] + 10000;
    }

    // Score quiet move
    else {
        // score 1st killer move
        if (pos->get_killer_move(0, pos->get_ply()) == move) {
            return 9000;
        }
        // score 2nd killer move
        else if (pos->get_killer_move(1, pos->get_ply()) == move) {
            return 8000;
        }
        // score history move
        else {
            return pos->get_history_score(get_move_piece(move), get_move_target(move));
        }
    }
}

// Sort moves in descending order
void sort_moves(const Board *pos, std::vector<Move>& move_list) {
    // Score all the moves
    for (int i = 0; i < move_list.size(); ++i) {
        move_list[i].score = score_move(pos, move_list[i].move);
    }

    // Sort the move list in descending order of scores
    std::sort(move_list.begin(), move_list.end(), [](const Move& a, const Move& b) {
        return a.score > b.score;
        });
}
