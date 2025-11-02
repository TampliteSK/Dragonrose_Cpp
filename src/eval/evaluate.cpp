// evaluate.cpp

#include "evaluate.hpp"

#include <algorithm>
#include <iostream>

#include "../chess/Board.hpp"
#include "../chess/attack.hpp"
#include "../chess/attackgen.hpp"
#include "../chess/bitboard.hpp"
#include "../datatypes.hpp"
#include "ScorePair.hpp"
#include "endgame.hpp"

// Function prototypes
static inline uint8_t get_phase(const Board *pos);

static inline int evaluate_pawns(const Board *pos, uint8_t pce, int phase);
static inline int evaluate_knights(const Board *pos, uint8_t pce, int phase);
static inline int evaluate_bishops(const Board *pos, uint8_t pce, int phase);
static inline int evaluate_rooks(const Board *pos, uint8_t pce, int phase);
static inline int evaluate_queens(const Board *pos, uint8_t pce, int phase);
static inline int evaluate_kings(const Board *pos, uint8_t pce, int phase);

static inline int16_t count_tempi(const Board *pos);
static inline int16_t evaluate_attacks(const Board *pos, Bitboard white_attacks[],
                                     Bitboard black_attacks[], int white_attackers[],
                                     int black_attackers[], int phase);

int evaluate_pos(const Board *pos) {
    int score = 0;
    int phase = get_phase(pos);

    Bitboard white_attacks[32] = {0ULL};
    Bitboard black_attacks[32] = {0ULL};
    int white_attackers[32] = {
        0};  // The pieces corresponding to each attack bitboard in piece_attacks
    int black_attackers[32] = {0};

    Bitboard pawns = pos->bitboards[wP] | pos->bitboards[bP];
    bool is_TB_endgame =
        count_bits(pos->occupancies[BOTH]) - pos->piece_num[wP] - pos->piece_num[bP] < 8;
    if (is_TB_endgame && count_bits(pawns) == 0) {
        if (check_material_draw(pos, phase)) {
            return endgame_noise(pos->hash_key % UINT32_MAX, 3);
        }
    }

    bool is_endgame = count_bits(pos->occupancies[BOTH]) < 8;

    score += count_material(pos, phase);
    // std::cout << "Material: " << count_material(pos) << "\n";

    // Get easily-accessible attacks in one-go to save time
    get_all_attacks(pos, WHITE, white_attacks, white_attackers);
    get_all_attacks(pos, BLACK, black_attacks, black_attackers);

    for (int colour = WHITE; colour <= BLACK; ++colour) {
        int8_t sign = (colour == WHITE) ? 1 : -1;
        uint8_t col_offset = (colour == WHITE) ? 0 : 6;

        score += evaluate_pawns(pos, wP + col_offset, phase) * sign;
        // std::cout << "    " << ascii_pieces[wP + col_offset] << ": " << evaluate_pawns(pos,
        // passers, wP + col_offset, phase) * sign << "\n";
        score += evaluate_knights(pos, wN + col_offset, phase) * sign;
        // std::cout << "    " << ascii_pieces[wN + col_offset] << ": " << evaluate_knights(pos, wN
        // + col_offset, phase) * sign << "\n";
        score += evaluate_bishops(pos, wB + col_offset, phase) * sign;
        // std::cout << "    " << ascii_pieces[wB + col_offset] << ": " << evaluate_bishops(pos, wB
        // + col_offset, phase) * sign << "\n";
        score += evaluate_rooks(pos, wR + col_offset, phase) * sign;
        // std::cout << "    " << ascii_pieces[wR + col_offset] << ": " << evaluate_rooks(pos, wR +
        // col_offset, phase) * sign << "\n";
        score += evaluate_queens(pos, wQ + col_offset, phase) * sign;
        // std::cout << "    " << ascii_pieces[wQ + col_offset] << ": " << evaluate_queens(pos, wQ +
        // col_offset, phase) * sign << "\n"; int old_score = score;
        if (colour == WHITE) {
            score += evaluate_kings(pos, wK, phase);
        } else {
            score -= evaluate_kings(pos, bK, phase);
        }
        // std::cout << "    " << ascii_pieces[wK + col_offset] << ": " << score - old_score <<
        // "\n";
    }
    // std::cout << "PSQT and co.: " << score - count_material(pos) << "\n";

    // Alternate phase formula for the rest of the function. Works better than one used for PSQT in
    // such cases
    int var_phase = count_bits(pos->occupancies[BOTH] &
                               ~(pos->bitboards[wP] | pos->bitboards[bP]));  // Values: [0, 16]

    // Tempi
    if (!is_endgame) {
        score += count_tempi(pos) * var_phase / 16;  // Towards endgame considering tempi is useless
        // std::cout << "Tempi: " << count_tempi(pos) * var_phase / 16 << "\n";
    }

    /*
            Piece activity / control
    */
    score += evaluate_attacks(pos, white_attacks, black_attacks, white_attackers, black_attackers, phase);
    // std::cout << "Activity: " << count_activity(white_attacks, black_attacks, white_attackers,
    // black_attackers) * 3 / 2 << "\n";

    // Bishop pair bonus
    if (pos->piece_num[wB] >= 2) score += bishop_pair;
    if (pos->piece_num[bB] >= 2) score -= bishop_pair;

    /*
            Endgame adjustments
    */

    // 50-move rule adjustment
    if (score < MATE_SCORE) {
        score = (score * (100 - pos->fifty_move)) / 100;
    }

    // Endgame noise adjustment
    /*
    if (is_endgame) {
            score += check_material_win(pos)
    }
    */

    // Perspective adjustment
    return score * ((pos->side == WHITE) ? 1 : -1);
}

/*
        Components
*/

// Calculates the middlegame weight for tapered eval.
// Evaluates to 64 at startpos
static inline uint8_t get_phase(const Board *pos) {
    // Caissa game phase formula (0.11e)
    // ~20 elo better than the original PesTO phase formula
    int game_phase =
        3 * (pos->piece_num[wN] + pos->piece_num[bN] + pos->piece_num[wB] + pos->piece_num[bB]);
    game_phase += 5 * (pos->piece_num[wR] + pos->piece_num[bR]);
    game_phase += 10 * (pos->piece_num[wQ] + pos->piece_num[bQ]);
    game_phase = std::min(game_phase, 64);  // Capped in case of early promotion

    return game_phase;
}

// Calculates the material from White's perspective
int count_material(const Board *pos, const uint8_t phase) {
    int sum = 0;

    for (int pce = wP; pce <= bK; ++pce) {
        int value = pos->piece_num[pce] *
                    (piece_values[pce].mg() * phase + piece_values[pce].eg() * (64 - phase)) / 64;
        sum += value * ((piece_col[pce] == WHITE) ? 1 : -1);
    }

    return sum;
}

static inline int compute_PSQT(uint8_t pce, uint8_t sq, int phase) {
    int score = 0;
    uint8_t col = piece_col[pce];
    if (col == WHITE) {
        score += (PSQT[piece_type[pce] - 1][sq].mg() * phase +
                  PSQT[piece_type[pce] - 1][sq].eg() * (64 - phase)) /
                 64;
    } else {
        score += (PSQT[piece_type[pce] - 1][Mirror64[sq]].mg() * phase +
                  PSQT[piece_type[pce] - 1][Mirror64[sq]].eg() * (64 - phase)) /
                 64;
    }
    return score;
}

/*
        Piece evaluation
*/

static inline int evaluate_pawns(const Board *pos, uint8_t pce, int phase) {
    int score = 0;
    Bitboard pawns = pos->bitboards[pce];
    uint8_t enemy_pce = (pce == wP) ? bP : wP;
    bool passers[8] = {false};

    while (pawns) {
        uint8_t sq = pop_ls1b(pawns);
        uint8_t col = piece_col[pce];
        uint8_t file = GET_FILE(sq);
        uint8_t reference_rank = (pce == wP) ? (7 - GET_RANK(sq)) : GET_RANK(sq);
        uint8_t reference_sq = (pce == wP) ? (sq - 8) : (sq + 8);
        score += compute_PSQT(pce, sq, phase);

        /*
                Pawn structure
        */

        // Passed pawn bonuses
        // 1) There are no enemy pawns on the same or adjacent file(s)
        // 2) The pawn on the same or adjacent file(s) are behind the pawn
        Bitboard passer_mask = (col == WHITE) ? white_passer_masks[sq] : black_passer_masks[sq];
        if ((passer_mask & pos->bitboards[enemy_pce]) == 0) {
            passers[file] = true;
            score += passer_bonus[reference_rank];
            // std::cout << "Passer detected at " << ascii_squares[sq] << " | Score: " <<
            // passer_bonus[reference_rank] << "\n";
        }

        // Isolated and backwards pawn penalties
        // Backwards pawn: No neighbouring pawns or they are further advanced, and square in front
        // is attacked by an opponent's pawn
        bool is_isolated = (pos->bitboards[pce] & adjacent_files[file]) == 0;
        bool is_stopped = pos->bitboards[enemy_pce] & pawn_attacks[col][reference_sq];
        bool is_backwards = false;
        if (is_stopped) {
            if (is_isolated) {
                is_backwards = true;
            } else {
                // There exists neighbouring pawns. Check if they are further advanced.
                bool advanced_neighbours = false;
                Bitboard pawn_mask = pos->bitboards[pce] & adjacent_files[file];
                while (pawn_mask) {
                    uint8_t neighbour = pop_ls1b(pawn_mask);
                    if ((col == WHITE && GET_RANK(neighbour) <= GET_RANK(sq)) ||
                        (col == BLACK && GET_RANK(neighbour) >= GET_RANK(sq))) {
                        advanced_neighbours = true;  // There is at least 1 neighbour less or
                                                     // equally advanced. Not a backwards pawn.
                    }
                }
                if (!advanced_neighbours) {
                    is_backwards = true;
                }
            }

            if (is_backwards) {
                score -= backwards_pawn;
            }
        } else if (is_isolated) {
            score -= isolated_pawn;
        }
    }

    // File-based features
    for (int file = FILE_A; file <= FILE_H; ++file) {
        // Stacked pawn penalties
        uint8_t stacked_count = count_bits(pos->bitboards[pce] & file_masks[file]);
        if (stacked_count > 1) {
            score -= stacked_pawn * (stacked_count - 1);  // Scales with the number of pawns stacked
        }

        // Connected passer bonuses
        if (file >= FILE_A) {
            if (passers[file] && passers[file - 1]) {
                score += connected_passers;
            }
        }
    }

    return score;
}

static inline int evaluate_knights(const Board *pos, uint8_t pce, int phase) {
    int score = 0;
    Bitboard knights = pos->bitboards[pce];

    while (knights) {
        uint8_t sq = pop_ls1b(knights);
        score += compute_PSQT(pce, sq, phase);
    }

    return score;
}

static inline int evaluate_bishops(const Board *pos, uint8_t pce, int phase) {
    int score = 0;
    Bitboard bishops = pos->bitboards[pce];
    uint8_t col = piece_col[pce];

    while (bishops) {
        uint8_t sq = pop_ls1b(bishops);
        uint8_t file = GET_FILE(sq);
        score += compute_PSQT(pce, sq, phase);

        // Penalty for bishops blocking centre pawns
        if (file == FILE_D || file == FILE_E) {
            Bitboard pawn_mask = pos->bitboards[pce] & file_masks[file];
            uint8_t pawn_sq = pop_ls1b(pawn_mask);
            int8_t sign = (piece_col[pce] == WHITE) ? -1 : 1;
            if (sq - pawn_sq == sign * 8) {
                score -= bishop_blocks_ctrpawn;
            }
        }

        // Bonus for pressuring enemy pieces
        Bitboard mask = get_bishop_attacks(sq, pos->occupancies[BOTH]) & pos->occupancies[col ^ 1];
        score += count_bits(mask) * bishop_attacks_piece;

        // Bonus for forming a battery with a queen
        uint8_t ally_queen = (col == WHITE) ? wQ : bQ;
        Bitboard queen_mask =
            pos->bitboards[ally_queen] & get_bishop_attacks(sq, pos->occupancies[BOTH]);
        if (queen_mask) {
            score += battery;
        }
    }

    return score;
}

static inline int evaluate_rooks(const Board *pos, uint8_t pce, int phase) {
    int score = 0;
    Bitboard rooks = pos->bitboards[pce];
    uint8_t col = piece_col[pce];
    bool stacked_rooks = false;

    while (rooks) {
        uint8_t sq = pop_ls1b(rooks);
        uint8_t file = GET_FILE(sq);
        uint8_t ally_pawns = (pce == wR) ? wP : bP;
        uint8_t enemy_pawns = (pce == wR) ? bP : wP;
        score += compute_PSQT(pce, sq, phase);

        // Bonus for taking semi-open and open files
        if ((pos->bitboards[ally_pawns] & file_masks[file]) == 0) {
            Bitboard enemy_pawns_on_file = pos->bitboards[enemy_pawns] & file_masks[file];
            if (enemy_pawns_on_file == 0) {
                score += rook_open_file;
            } else {
                score += rook_semiopen_file;
            }
        }

        // Bonus for pressuring enemy pieces
        Bitboard mask = get_rook_attacks(sq, pos->occupancies[BOTH]) & pos->occupancies[col ^ 1];
        score += count_bits(mask) * rook_attacks_piece;

        // Bonus for forming a battery with a queen or another rook
        uint8_t ally_queen = (col == WHITE) ? wQ : bQ;
        Bitboard queen_mask =
            pos->bitboards[ally_queen] & get_rook_attacks(sq, pos->occupancies[BOTH]);
        if (queen_mask) {
            score += battery;
        }

        if (!stacked_rooks) {
            Bitboard rook_mask = pos->bitboards[pce] & file_masks[file];
            if (count_bits(rook_mask) > 2) {
                score += battery * (count_bits(rook_mask) - 1);
                stacked_rooks = true;  // Prevent overcounting
            }
        }
    }

    return score;
}

static inline int evaluate_queens(const Board *pos, uint8_t pce, int phase) {
    int score = 0;
    Bitboard queens = pos->bitboards[pce];
    uint8_t col = piece_col[pce];

    while (queens) {
        uint8_t sq = pop_ls1b(queens);
        uint8_t file = GET_FILE(sq);
        uint8_t ally_pawns = (pce == wR) ? wP : bP;
        uint8_t enemy_pawns = (pce == wR) ? bP : wP;
        score += compute_PSQT(pce, sq, phase);

        // Bonus for taking semi-open and open files
        if ((pos->bitboards[ally_pawns] & file_masks[file]) == 0) {
            Bitboard enemy_pawns_on_file = pos->bitboards[enemy_pawns] & file_masks[file];
            if (enemy_pawns_on_file == 0) {
                score += queen_open_file;
            } else {
                score += queen_semiopen_file;
            }
        }

        // Bonus for pressuring enemy pieces
        Bitboard mask = get_queen_attacks(sq, pos->occupancies[BOTH]) & pos->occupancies[col ^ 1];
        score += count_bits(mask) * queen_attacks_piece;
    }

    return score;
}

/*
        King evaluation
*/

static inline int evaluate_pawn_shield(const Board *pos, uint8_t pce, uint8_t king_sq,
                                       int var_phase) {
    // [0]: starting sq
    // [1]: moved 1 sq
    // [2]: moved 2 sq
    // [3]: moved 3+ sq / dead / doubled to another file
    const int pawn_shield[4] = {0, -5, -15, -50};
    uint8_t king_file = GET_FILE(king_sq);
    uint8_t king_rank = GET_RANK(king_sq);
    uint8_t king_colour = piece_col[pce];

    // Criteria for uncastled king: on a central file OR too far advanced
    bool uncastled_king = king_file == FILE_D || king_file == FILE_E;
    if (king_colour == WHITE) {
        uncastled_king |= king_rank <= RANK_4;
    } else {
        uncastled_king |= king_rank >= RANK_5;
    }

    int score = 0;

    // Pawn shield only applies if the king is castled
    if (!uncastled_king) {
        uint8_t ally_pawns = (king_colour == WHITE) ? wP : bP;
        Bitboard shield_mask =
            generate_shield_zone(king_sq, king_colour) & pos->bitboards[ally_pawns];

        for (int file = king_file - 1; file <= king_file + 1; ++file) {
            if (file >= FILE_A && file <= FILE_H) {
                Bitboard shield_file = shield_mask & file_masks[file];

                // There is no pawn here (either dead, too advanced or doubled)
                if (shield_file == 0ULL) {
                    score += pawn_shield[3];
                }
                // Stacked pawns on file
                else if (count_bits(shield_file) > 1) {
                    score += pawn_shield[1];  // Becomes a target like a single-advanced pawn
                }
                // One pawn on file (normal case)
                else {
                    uint8_t pawn_sq = pop_ls1b(shield_file);
                    uint8_t rank_distance = abs(king_rank - GET_RANK(pawn_sq));
                    score += pawn_shield[rank_distance - 1];
                }
            }
        }
    }

    return score * var_phase / 16;
}

static inline int evaluate_king_safety(const Board *pos, uint8_t pce, uint8_t king_sq,
                                       int var_phase) {
    // Importance of king safety decreases with less material on the board
    int score = 0;
    score += evaluate_pawn_shield(pos, pce, king_sq, var_phase);
    return score;
}

// Rewarding active kings and punishing immobile kings to assist mates
static inline int16_t king_mobility(const Board *pos, uint8_t pce, uint8_t king_sq, int var_phase) {
    const int mobility_bonus[9] = {-75, -50, -33, -25, 0, 5, 10, 11, 12};

    Bitboard attacks = king_attacks[king_sq];
    uint8_t attacker = piece_col[pce] ^ 1;
    uint8_t mobile_squares = 0;

    while (attacks) {
        uint8_t sq = pop_ls1b(attacks);
        if (!is_square_attacked(pos, sq, attacker)) {
            mobile_squares++;
        }
    }

    return mobility_bonus[mobile_squares] * (20 - var_phase) / 16;
}

static inline int evaluate_kings(const Board *pos, uint8_t pce, int phase) {
    int score = 0;
    uint8_t sq = pos->king_sq[piece_col[pce]];
    score += compute_PSQT(pce, sq, phase);
    int var_phase =
        count_bits(pos->occupancies[BOTH] &
                   ~(pos->bitboards[wP] |
                     pos->bitboards[bP]));  // Better phasing formula in this case than one for PSQT
    score += evaluate_king_safety(pos, pce, sq, var_phase);
    if (var_phase <= 12) {
        score += king_mobility(pos, pce, sq, var_phase);
    }
    return score;
}

/*
        Misc components
*/

// Draft tempo formula
// Gives a base value for the player up (a) tempo/tempi, proportional the number of developed
// pieces_BB + central pawns moved The evaluation will be increased based on differences on other
// factors e.g. The tempo is used to develop the g knight to f3 => 20 + (PSQT_knight_MG[F3] -
// PSQT_knight_MG[G1]) Only applies to opening / early-middlegame
static inline int16_t count_tempi(const Board *pos) {
    uint8_t white_adv = 20;  // White's first-move advantage
    uint8_t tempo = 8;       // Value of a typical tempo
    Bitboard UNDEVELOPED_WHITE_ROOKS = (1ULL << a1) | (1ULL << h1);
    Bitboard UNDEVELOPED_BLACK_ROOKS = (1ULL << a8) | (1ULL << h8);

    int8_t net_developed_pieces = 0;
    Bitboard white_DE_pawns = pos->bitboards[wP] & (file_masks[FILE_D] | file_masks[FILE_E]);
    Bitboard white_NBQ = pos->bitboards[wN] | pos->bitboards[wB] | pos->bitboards[wQ];
    Bitboard black_DE_pawns = pos->bitboards[bP] & (file_masks[FILE_D] | file_masks[FILE_E]);
    Bitboard black_NBQ = pos->bitboards[bN] | pos->bitboards[bB] | pos->bitboards[bQ];

    net_developed_pieces += count_bits(white_NBQ & DEVELOPMENT_MASK);  // wN, wB, wQ
    net_developed_pieces += count_bits(pos->bitboards[wR] & ~UNDEVELOPED_WHITE_ROOKS);  // wR
    net_developed_pieces +=
        count_bits(white_DE_pawns & ~bits_between_squares(d2, e2));    // wP (centre pawns)
    net_developed_pieces -= count_bits(black_NBQ & DEVELOPMENT_MASK);  // bN, bB, bQ
    net_developed_pieces -= count_bits(pos->bitboards[bR] & ~UNDEVELOPED_BLACK_ROOKS);  // bR
    net_developed_pieces -=
        count_bits(black_DE_pawns & ~bits_between_squares(d7, e7));  // bP (centre pawns)

    return ((pos->side == WHITE) ? white_adv : 0) + net_developed_pieces * tempo;
}

// Using attack bitboards to evaluate piece activity and king safety
// Activity is based on number of attacked squares, based on jk_182's Lichess article:
// https://lichess.org/@/jk_182/blog/calculating-piece-activity/FAOY6Ii7
static inline int16_t evaluate_attacks(const Board *pos, Bitboard white_attacks[],
                                     Bitboard black_attacks[], int white_attackers[],
                                     int black_attackers[], int phase) {

    // int mobility_bonus = (static_mobility.mg() * phase + static_mobility.eg() * (64 - phase)) / 64;
    // int activity = 0;
    int mobility = 0;
    uint8_t attack_bits = 0;

    // Attacks: The attack bitboards
    // Attackers: Piece types corresponding to each attack bitboard
    Bitboard virtual_queen_w = get_queen_attacks(pos->king_sq[WHITE], pos->occupancies[BOTH]);
    Bitboard virtual_queen_b = get_queen_attacks(pos->king_sq[BLACK], pos->occupancies[BOTH]);
    int attack_units[13] = {0, 1, 2, 2, 3, 5, 0, 1, 2, 2, 3, 5, 0};
    int total_white_units = 0, total_black_units = 0;

    for (int i = 0; i < 32; ++i) {
        if (white_attackers[i] != EMPTY) {
            attack_bits = count_bits(white_attacks[i]);

            // === MOBILITY ===
            // Give malus/bonus based on how many squares are controlled by the piece
            uint8_t pce = piece_type[white_attackers[i]]; // Guaranteed to be non-pawn, non-EMPTY
            ScorePair pair = MOBILITY_VALUES[pce - 2][attack_bits];
            mobility += (pair.mg() * phase + pair.eg() * (64 - phase)) / 64;

            // === KING SAFETY ===
            Bitboard zone_attacks = white_attacks[i] & virtual_queen_b;
            if (zone_attacks != 0) {
                total_white_units += attack_units[white_attackers[i]] * count_bits(zone_attacks);
            }
        }

        if (black_attackers[i] != EMPTY) {
            attack_bits = count_bits(black_attacks[i]);

            // === MOBILITY ===
            uint8_t pce = piece_type[black_attackers[i]];
            ScorePair pair = MOBILITY_VALUES[pce - 2][attack_bits];
            mobility -= (pair.mg() * phase + pair.eg() * (64 - phase)) / 64;

            // === KING SAFETY ===
            Bitboard zone_attacks = black_attacks[i] & virtual_queen_w;
            if (zone_attacks != 0) {
                total_black_units += attack_units[black_attackers[i]] * count_bits(zone_attacks);
            }
        }
    }

    int var_phase = count_bits(pos->occupancies[BOTH] & ~(pos->bitboards[wP] | pos->bitboards[bP]));

    int white_king = -safety_table[std::min(total_black_units, 99)] * var_phase / 16;
    int black_king = -safety_table[std::min(total_white_units, 99)] * var_phase / 16;
    int king_attacks = white_king - black_king;

    return mobility + king_attacks;
}