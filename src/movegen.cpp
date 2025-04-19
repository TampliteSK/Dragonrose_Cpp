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
#include "makemove.hpp"

/*
        Most Valuable Victim & Least Valuable Attacker

    (Victims) Pawn Knight Bishop   Rook  Queen   King
  (Attackers)
        Pawn   105    205    305    405    505    605
      Knight   104    204    304    404    504    604
      Bishop   103    203    303    403    503    603
        Rook   102    202    302    402    502    602
       Queen   101    201    301    401    501    601
        King   100    200    300    400    500    600

*/

// MVV LVA [attacker][victim]
static int mvv_lva[12][12] = {
    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};

/*
    === Move Ordering ===
    PV Move                                                     10,000,000
    Promotion (Queen) *                                          5,000,000
    Captures (inc. enpas) + MVV-LVA                              2,000,000 - 2,000,606
    Killers (moves that lead to beta cut-off but not captures)     900,000 / 950,000
    O-O, O-O-O                                                     750,000
    Checks *                                                     ( 500,000 )                     
    Promotion (Knight) *                                           300,000
    Promotion (Rook) *                                             200,000
    Promotion (Bishop) *                                           100,000
    Pawn move (non-capture, non-promo) *                            50,000
    HistoryScore                                                  >=35,000
    no_score                                                        35,000

*: Requires gainer tests
*/

/*
    Move generation
*/

// add move to the move list
static inline void add_move(std::vector<Move>& move_list, int move, int score) {
    Move move_struct = { move, score };
    move_list.push_back(move_struct);
}

// Movegen function forked from BBC by Maksim Korzh (Code Monkey King)
void generate_moves(const Board *pos, std::vector<Move>& move_list, bool noisy_only) {

    uint8_t source_square, target_square;
    uint8_t side = pos->side;
    uint8_t col_offset = (side == WHITE) ? 0 : 6;
    Bitboard bitboard, attacks; // define current piece's bitboard copy & it's attacks

    for (int piece = wP + col_offset; piece <= wK + col_offset; piece++) {
        bitboard = pos->bitboards[piece];

        // White pawns
        if (piece == wP) {
            // loop over white pawns within white pawn bitboard
            while (bitboard) {
                source_square = pop_ls1b(bitboard);
                target_square = source_square - 8;

                // Generate quiet pawn moves
                if (!noisy_only) {
                    if (target_square >= a8 && !GET_BIT(pos->occupancies[BOTH], target_square)) {

                        // pawn promotion
                        if (source_square >= a7 && source_square <= h7) {
                            add_move(move_list, encode_move(source_square, target_square, piece, wQ, 0, 0, 0, 0), 5'000'000);
                            add_move(move_list, encode_move(source_square, target_square, piece, wR, 0, 0, 0, 0), 200'000);
                            add_move(move_list, encode_move(source_square, target_square, piece, wB, 0, 0, 0, 0), 100'000);
                            add_move(move_list, encode_move(source_square, target_square, piece, wN, 0, 0, 0, 0), 300'000);
                        }
                        else {
                            // one square ahead pawn move
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0), 50'000);

                            // two squares ahead pawn move
                            if (source_square >= a2 && source_square <= h2 && !GET_BIT(pos->occupancies[BOTH], target_square - 8)) {
                                add_move(move_list, encode_move(source_square, target_square - 8, piece, 0, 0, 1, 0, 0), 50'000);
                            }
                        }
                    }
                }

                attacks = pawn_attacks[side][source_square] & pos->occupancies[BLACK];

                // generate pawn captures
                while (attacks) {

                    target_square = pop_ls1b(attacks);
                    int target_pce = pos->pieces[target_square];

                    // pawn promotion
                    if (source_square >= a7 && source_square <= h7) {
                        add_move(move_list, encode_move(source_square, target_square, piece, wQ, target_pce, 0, 0, 0), 5'000'000);
                        add_move(move_list, encode_move(source_square, target_square, piece, wR, target_pce, 0, 0, 0), 200'000);
                        add_move(move_list, encode_move(source_square, target_square, piece, wB, target_pce, 0, 0, 0), 100'000);
                        add_move(move_list, encode_move(source_square, target_square, piece, wN, target_pce, 0, 0, 0), 300'000);
                    }
                    else {
                        // Normal capture
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, target_pce, 0, 0, 0), 0);
                    }
                }

                // generate enpassant captures
                if (pos->enpas != NO_SQ) {
                    Bitboard enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << pos->enpas); // Check if enpassant is a valid capture
                    if (enpassant_attacks) {
                        int target_enpassant = pop_ls1b(enpassant_attacks);
                        add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, bP, 0, 1, 0), 0);
                    }
                }
            }
        }

        // Black pawn moves
        else if (piece == bP) {
            while (bitboard) {
                source_square = pop_ls1b(bitboard);
                target_square = source_square + 8;

                // Generate quiet pawn moves
                if (!noisy_only) {
                    if (target_square <= h1 && !GET_BIT(pos->occupancies[BOTH], target_square)) {

                        // pawn promotion
                        if (source_square >= a2 && source_square <= h2) {
                            add_move(move_list, encode_move(source_square, target_square, piece, bQ, 0, 0, 0, 0), 5'000'000);
                            add_move(move_list, encode_move(source_square, target_square, piece, bR, 0, 0, 0, 0), 200'000);
                            add_move(move_list, encode_move(source_square, target_square, piece, bB, 0, 0, 0, 0), 100'000);
                            add_move(move_list, encode_move(source_square, target_square, piece, bN, 0, 0, 0, 0), 300'000);
                        }
                        else {
                            // one square ahead pawn move
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0), 50'000);

                            // two squares ahead pawn move
                            if (source_square >= a7 && source_square <= h7 && !GET_BIT(pos->occupancies[BOTH], target_square + 8)) {
                                add_move(move_list, encode_move(source_square, target_square + 8, piece, 0, 0, 1, 0, 0), 50'000);
                            }

                        }
                    }
                }

                attacks = pawn_attacks[side][source_square] & pos->occupancies[WHITE];

                // generate pawn captures
                while (attacks) {

                    target_square = pop_ls1b(attacks);
                    int target_pce = pos->pieces[target_square];

                    // pawn promotion
                    if (source_square >= a2 && source_square <= h2) {
                        add_move(move_list, encode_move(source_square, target_square, piece, bQ, target_pce, 0, 0, 0), 5'000'000);
                        add_move(move_list, encode_move(source_square, target_square, piece, bR, target_pce, 0, 0, 0), 200'000);
                        add_move(move_list, encode_move(source_square, target_square, piece, bB, target_pce, 0, 0, 0), 100'000);
                        add_move(move_list, encode_move(source_square, target_square, piece, bN, target_pce, 0, 0, 0), 300'000);
                    }
                    else {
                        // Normal capture
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, target_pce, 0, 0, 0), 0);
                    }
                }

                // generate enpassant captures
                if (pos->enpas != NO_SQ) {
                    Bitboard enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << pos->enpas);
                    if (enpassant_attacks) {
                        int target_enpassant = pop_ls1b(enpassant_attacks);
                        add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, wP, 0, 1, 0), 0);
                    }
                }
            }
        }

        // Everything else
        else {
            // White castling moves
            if (!noisy_only && piece == wK) {
                // King side castling
                if (pos->castle_perms & WKCA) {
                    if (pos->pieces[f1] == EMPTY && pos->pieces[g1] == EMPTY) {
                        if (!is_square_attacked(pos, e1, BLACK) && !is_square_attacked(pos, f1, BLACK)) {
                            add_move(move_list, encode_move(e1, g1, piece, 0, 0, 0, 0, 1), 750'000);
                        }
                    }
                }
                // Queen side castling
                if (pos->castle_perms & WQCA) {
                    if (pos->pieces[d1] == EMPTY && pos->pieces[c1] == EMPTY && pos->pieces[b1] == EMPTY) {
                        if (!is_square_attacked(pos, e1, BLACK) && !is_square_attacked(pos, d1, BLACK)) {
                            add_move(move_list, encode_move(e1, c1, piece, 0, 0, 0, 0, 1), 750'000);
                        }
                    }
                }
            }
            // Black castling moves
            else if (!noisy_only && piece == bK) {
                // King side castling
                if (pos->castle_perms & BKCA) {
                    if (pos->pieces[f8] == EMPTY && pos->pieces[g8] == EMPTY) {
                        if (!is_square_attacked(pos, e8, WHITE) && !is_square_attacked(pos, f8, WHITE)) {
                            add_move(move_list, encode_move(e8, g8, piece, 0, 0, 0, 0, 1), 750'000);
                        }
                    }
                }
                // Queen side castling
                if (pos->castle_perms & BQCA) {
                    if (pos->pieces[d8] == EMPTY && pos->pieces[c8] == EMPTY && pos->pieces[b8] == EMPTY) {
                        if (!is_square_attacked(pos, e8, WHITE) && !is_square_attacked(pos, d8, WHITE)) {
                            add_move(move_list, encode_move(e8, c8, piece, 0, 0, 0, 0, 1), 750'000);
                        }
                    }
                }
            }
            
            while (bitboard) {
                source_square = pop_ls1b(bitboard);
                attacks = get_piece_attacks(pos, piece, source_square) & ((side == WHITE) ? ~pos->occupancies[WHITE] : ~pos->occupancies[BLACK]);

                while (attacks) {
                    target_square = pop_ls1b(attacks);
                    int target_pce = pos->pieces[target_square];

                    // Capture move
                    if (piece_col[pos->pieces[target_square]] == (side ^ 1)) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, target_pce, 0, 0, 0), 0);
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
    int captured = get_move_captured(move);
    if (captured) {
        return mvv_lva[get_move_piece(move) - 1][captured - 1] + 2'000'000;
    }

    // Score quiet move
    else {
        // score 1st killer move
        if (pos->killer_moves[0][pos->ply] == move) {
            return 950'000;
        }
        // score 2nd killer move
        else if (pos->killer_moves[1][pos->ply] == move) {
            return 900'000;
        }
        // score history move
        else {
            return pos->history_moves[get_move_piece(move)][get_move_target(move)];
        }
    }
}

// Sort moves in descending order
void sort_moves(const Board *pos, std::vector<Move>& move_list) {
    // Score all the moves
    for (int i = 0; i < (int)move_list.size(); ++i) {
        move_list[i].score += score_move(pos, move_list[i].move);
    }

    // Sort the move list in descending order of scores
    std::sort(move_list.begin(), move_list.end(), [](const Move& a, const Move& b) {
        return a.score > b.score;
        });
}

// Determine is a move is possible in a given position
bool move_exists(Board* pos, const int move) {

    std::vector<Move> list;
    generate_moves(pos, list, false);

    for (int i = 0; i < (int)list.size(); ++i) {

        if (!make_move(pos, list[i].move)) {
            continue;
        }
        take_move(pos);
        if (list[i].move == move) {
            return true;
        }
    }
    return false;
}