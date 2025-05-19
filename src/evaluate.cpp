// evaluate.cpp

#include <algorithm>
#include <iostream>
#include "evaluate.hpp"
#include "datatypes.hpp"
#include "attack.hpp"
#include "attackgen.hpp"
#include "Board.hpp"
#include "bitboard.hpp"
#include "endgame.hpp"

Bitboard piece_attacks_white[32] = { 0ULL };
Bitboard piece_attacks_black[32] = { 0ULL };
int white_attackers[32] = { 0 }; // The pieces corresponding to each attack bitboard in piece_attacks
int black_attackers[32] = { 0 };

// PesTO Material
uint16_t piece_value_MG[13] = { 0, 82, 337, 365, 477, 1025, 30000, 82, 337, 365, 477, 1025, 30000 };
uint16_t piece_value_EG[13] = { 0, 94, 281, 297, 512,  936, 30000, 94, 281, 297, 512,  936, 30000 };

// Function prototypes
static inline int get_phase(const Board* pos);

static inline int evaluate_pawns(const Board* pos, uint8_t pce, int phase); 
static inline int evaluate_knights(const Board* pos, uint8_t pce, int phase);
static inline int evaluate_bishops(const Board* pos, uint8_t pce, int phase);
static inline int evaluate_rooks(const Board* pos, uint8_t pce, int phase);
static inline int evaluate_queens(const Board* pos, uint8_t pce, int phase);
static inline int evaluate_kings(const Board* pos, uint8_t pce, int phase);

static inline int16_t count_tempi(const Board* pos);
static inline int16_t count_activity();

int evaluate_pos(const Board* pos) {

	Bitboard pawns = pos->bitboards[wP] | pos->bitboards[bP];
	bool is_TB_endgame = count_bits(pos->occupancies[BOTH]) - pos->piece_num[wP] - pos->piece_num[bP] < 8;
	if (is_TB_endgame && count_bits(pawns) == 0) {
		if (check_material_draw(pos)) {
			return endgame_noise(pos->hash_key % UINT32_MAX, 3);
		}
	}

	bool is_endgame = count_bits(pos->occupancies[BOTH]) < 8;
	int score = 0;
	int phase = get_phase(pos);

	score += count_material(pos);
	// std::cout << "Material: " << count_material(pos) << "\n";

	// Get easily-accessible attacks in one-go to save time
	get_all_attacks(pos, WHITE, piece_attacks_white, white_attackers, true);
	get_all_attacks(pos, BLACK, piece_attacks_black, black_attackers, true);

	for (int colour = WHITE; colour <= BLACK; ++colour) {
		int8_t sign = (colour == WHITE) ? 1 : -1;
		uint8_t col_offset = (colour == WHITE) ? 0 : 6;
		score += evaluate_pawns(pos, wP + col_offset, phase) * sign;
		score += evaluate_knights(pos, wN + col_offset, phase) * sign;
		score += evaluate_bishops(pos, wB + col_offset, phase) * sign;
		score += evaluate_rooks(pos, wR + col_offset, phase) * sign;
		score += evaluate_queens(pos, wQ + col_offset, phase) * sign;
		score += evaluate_kings(pos, wK + col_offset, phase) * sign;
	}
	// std::cout << "PSQT and co.: " << score - count_material(pos) << "\n";

	// Tempi
	score += count_tempi(pos) * phase / 64; // Towards endgame considering tempi is useless
	// std::cout << "Tempi: " << count_tempi(pos) * phase / 64 << "\n";

	/*
		Piece activity / control
	*/
	score += count_activity();
	// std::cout << "Activity: " << count_activity() << "\n";
	
	// Measure centre control
	int control = 0;
	int centre_squares[] = { d4, d5, e4, e5 };
	for (int sq : centre_squares) {
		control += get_square_control(pos, sq, WHITE);
	}
	score += control / 2;
	// std::cout << "Centre control: " << control / 2 << "\n";
	
	// Bishop pair bonus
	if (pos->piece_num[wB] > 2) score += bishop_pair;
	if (pos->piece_num[bB] > 2) score -= bishop_pair;
	
	/*
		Endgame adjustments
	*/

	// 50-move rule adjustment
	if (score < MATE_SCORE) {
		score = (score * (100 - pos->fifty_move)) / 100;
	}

	// Endgame noise adjustment
	if (is_endgame) {
		score += /*check_material_win(pos) +*/ endgame_noise(pos->hash_key % UINT32_MAX, 3);
	}

	// Perspective adjustment
	// std::cout << "Final eval: " << score * ((pos->side == WHITE) ? 1 : -1) << "\n\n";
	return score * ((pos->side == WHITE) ? 1 : -1);
}

/*
	Components
*/

// Calculates the middlegame weight for tapered eval.
// Evaluates to 64 at startpos
static inline int get_phase(const Board* pos) {
	// Caissa game phase formula (0.11e)
	// Performs about 200 elo better than PesTO's own tapered eval and directly scaling to material
	int game_phase = 3 * (pos->piece_num[wN] + pos->piece_num[bN] + pos->piece_num[wB] + pos->piece_num[bB]);
	game_phase += 5 * (pos->piece_num[wR] + pos->piece_num[bR]);
	game_phase += 10 * (pos->piece_num[wQ] + pos->piece_num[bQ]);
	game_phase = std::min(game_phase, 64); // Capped in case of early promotion

	return game_phase;
}

// Calculates the material from White's perspective
int count_material(const Board* pos) {
	int sum = 0;
	int phase = get_phase(pos);

	for (int pce = wP; pce <= bK; ++pce) {
		int value = pos->piece_num[pce] * ( piece_value_MG[pce] * phase + piece_value_EG[pce] * (64 - phase)) / 64;
		sum += value * ((piece_col[pce] == WHITE) ? 1 : -1);
	}

	return sum;
}

static inline int compute_PSQT(uint8_t pce, uint8_t sq, int phase) {
	int score = 0;
	uint8_t col = piece_col[pce];
	if (col == WHITE) {
		score += (PSQT_MG[piece_type[pce] - 1][sq] * phase + PSQT_EG[piece_type[pce] - 1][sq] * (64 - phase)) / 64;
	}
	else {
		score += (PSQT_MG[piece_type[pce] - 1][Mirror64[sq]] * phase + PSQT_EG[piece_type[pce] - 1][Mirror64[sq]] * (64 - phase)) / 64;
	}
	return score;
}

/*
	Piece evaluation
*/

static inline int evaluate_pawns(const Board* pos, uint8_t pce, int phase) {
	int score = 0;
	Bitboard pawns = pos->bitboards[pce];
	uint8_t enemy_pce = (pce == wP) ? bP : wP;

	while (pawns) {
		uint8_t sq = pop_ls1b(pawns);
		uint8_t col = piece_col[pce];
		uint8_t file = GET_FILE(sq);
		uint8_t reference_rank = (pce == wP) ? (8 - GET_RANK(sq)) : (GET_RANK(sq));
		uint8_t reference_sq = (pce == wP) ? (sq - 8) : (sq + 8);
		score += compute_PSQT(pce, sq, phase);

		/*
			Pawn structure
		*/

		// Passed pawn bonuses
		int passers = 0;
		if (file == FILE_A) {
			if (((pos->bitboards[enemy_pce] & (file_masks[FILE_A] | file_masks[FILE_B])) == 0)) {
				passers += passer_bonus[reference_rank];
			}
		}
		else if (file == FILE_H) {
			if (((pos->bitboards[enemy_pce] & (file_masks[FILE_G] | file_masks[FILE_H])) == 0)) {
				passers += passer_bonus[reference_rank];
			}
		}
		else if ((pos->bitboards[enemy_pce] & (file_masks[file - 1] | file_masks[file] | file_masks[file + 1])) == 0) {
			passers += passer_bonus[reference_rank];
		}
		score += passers;

		// Isolated and backwards pawn penalties
		// Backwards pawn: No neighbouring pawns or they are further advanced, and square in front is attacked by an opponent's pawn 
		bool is_isolated = (pos->bitboards[pce] & adjacent_files[file]) == 0;
		bool is_stopped = pos->bitboards[enemy_pce] & pawn_attacks[col][reference_sq];
		bool is_backwards = false;
		if (is_stopped) {
			if (is_isolated) {
				is_backwards = true;
			}
			else {
				// There exists neighbouring pawns. Check if they are further advanced.
				bool advanced_neighbours = false;
				Bitboard pawn_mask = pos->bitboards[pce] & adjacent_files[file];
				while (pawn_mask) {
					uint8_t neighbour = pop_ls1b(pawn_mask);
					if ((col == WHITE && GET_RANK(neighbour) >= GET_RANK(sq)) ||
						(col == BLACK && GET_RANK(neighbour) <= GET_RANK(sq)) ) {
						advanced_neighbours = true; // There is at least 1 neighbour less or equally advanced. Not a backwards pawn.
					}
				}
				if (!advanced_neighbours) {
					is_backwards = true;
				}
			}
			
			if (is_backwards) {
				score -= backwards_pawn;
			}
		}
		else if (is_isolated) {
			score -= isolated_pawn;
		}

		// Stacked pawn penalties
		uint64_t stacked_mask = pos->bitboards[pce] & file_masks[file];
		uint8_t stacked_count = count_bits(stacked_mask);
		if (stacked_count > 1) {
			score -= stacked_pawn * (stacked_count - 1) / stacked_count; // Scales with the number of pawns stacked. Division to make sure it's not overcounted.
		}

	}

	return score;
}

static inline int evaluate_knights(const Board* pos, uint8_t pce, int phase) {

	int score = 0;
	Bitboard knights = pos->bitboards[pce];

	while (knights) {
		uint8_t sq = pop_ls1b(knights);
		score += compute_PSQT(pce, sq, phase);
	}

	return score;
}

static inline int evaluate_bishops(const Board* pos, uint8_t pce, int phase) {

	int score = 0;
	Bitboard bishops = pos->bitboards[pce];

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
	}

	return score;
}

static inline int evaluate_rooks(const Board* pos, uint8_t pce, int phase) {

	int score = 0;
	Bitboard rooks = pos->bitboards[pce];

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
			}
			else {
				score += rook_semiopen_file;
			}
		}
	}

	return score;
}

static inline int evaluate_queens(const Board* pos, uint8_t pce, int phase) {

	int score = 0;
	Bitboard queens = pos->bitboards[pce];

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
			}
			else {
				score += queen_semiopen_file;
			}
		}
	}

	return score;
}

/*
	King evaluation
*/

// Attack units
static inline int evaluate_king_attacks(const Board* pos, uint8_t pce, uint8_t king_sq) {

	uint8_t attacking_col = piece_col[pce] ^ 1;
	Bitboard virtual_queen = get_queen_attacks(king_sq, pos->occupancies[BOTH]);
	Bitboard *attacks = (attacking_col == WHITE) ? piece_attacks_white : piece_attacks_black;
	int *attackers = (attacking_col == WHITE) ? white_attackers : black_attackers;

	int attack_units[13] = { 0, 1, 2, 2, 3, 5, 0, 1, 2, 2, 3, 5, 0 };
	int total_units = 0;
	
	for (int i = 0; i < 32; ++i) {
		if (attackers[i] == EMPTY) {
			break;
		}

		Bitboard zone_attacks = attacks[i] & virtual_queen;
		total_units += attack_units[attackers[i]] * count_bits(zone_attacks);
	}

	return -safety_table[std::min(total_units, 99)];
}

static inline int evaluate_pawn_shield(const Board* pos, uint8_t pce, uint8_t king_sq) {
	// [0]: starting sq
	// [1]: moved 1 sq
	// [2]: moved 2 sq
	// [3]: moved 3+ sq / dead / doubled to another file
	const int pawn_shield[4] = { 0, -5, -15, -50 };
	uint8_t king_file = GET_FILE(king_sq);
	uint8_t king_rank = GET_RANK(king_sq);
	uint8_t king_colour = piece_col[pce];

	int score = 0;
	// Criteria for uncastled king: on a central file OR too far advanced
	bool uncastled_king = king_file == FILE_D || king_file == FILE_E;
	if (king_colour == WHITE) {
		uncastled_king |= king_rank <= RANK_4;
	}
	else {
		uncastled_king |= king_rank >= RANK_5;
	}

	// Pawn shield only applies if the king is castled
	if (!uncastled_king) {
		uint8_t ally_pawns = (king_colour == WHITE) ? wP : bP;
		Bitboard shield_mask = generate_shield_zone(king_sq, king_colour) & pos->bitboards[ally_pawns];
		
		for (int file = king_file - 1; file <= king_file + 1; ++file) {
			if (file >= FILE_A && file <= FILE_H) {
				Bitboard shield_file = shield_mask & file_masks[file];

				// There is no pawn here (either dead, too advanced or doubled)
				if (shield_file == 0ULL) {
					score += pawn_shield[3];
				}
				// Stacked pawns on file
				else if (count_bits(shield_file) > 1) {
					score += pawn_shield[1]; // Becomes a target like a single-advanced pawn
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

	return score;
}

static inline int evaluate_king_safety(const Board* pos, uint8_t pce, uint8_t king_sq, int phase) {

	int score = 0;
	score += evaluate_king_attacks(pos, pce, king_sq);
	score += evaluate_pawn_shield(pos, pce, king_sq);
	return score * phase / 64; // Importance of king safety decreases with less material on the board
}

static inline int evaluate_kings(const Board* pos, uint8_t pce, int phase) {
	int score = 0;
	uint8_t sq = pos->king_sq[piece_col[pce]];
	score += compute_PSQT(pce, sq, phase);
	score += evaluate_king_safety(pos, pce, sq, phase);
	return score;
}

/*
	Misc components
*/

// Draft tempo formula
// Gives a base value for the player up (a) tempo/tempi, proportional the number of developed pieces_BB + central pawns moved
// The evaluation will be increased based on differences on other factors
// e.g. The tempo is used to develop the g knight to f3 => 20 + (PSQT_knight_MG[F3] - PSQT_knight_MG[G1])
// Only applies to opening / early-middlegame
static inline int16_t count_tempi(const Board* pos) {
	uint8_t white_adv = 20; // White's first-move advantage
	uint8_t tempo = 8;		// Value of a typical tempo
	Bitboard UNDEVELOPED_WHITE_ROOKS = (1ULL << a1) | (1ULL << h1);
	Bitboard UNDEVELOPED_BLACK_ROOKS = (1ULL << a8) | (1ULL << h8);

	int8_t net_developed_pieces = 0;
	Bitboard white_DE_pawns = pos->bitboards[wP] & (file_masks[FILE_D] | file_masks[FILE_E]);
	Bitboard white_NBQ = pos->bitboards[wN] | pos->bitboards[wB] | pos->bitboards[wQ];
	Bitboard black_DE_pawns = pos->bitboards[bP] & (file_masks[FILE_D] | file_masks[FILE_E]);
	Bitboard black_NBQ = pos->bitboards[bN] | pos->bitboards[bB] | pos->bitboards[bQ];

	net_developed_pieces += count_bits(white_NBQ & DEVELOPMENT_MASK);					// wN, wB, wQ
	net_developed_pieces += count_bits(pos->bitboards[wR] & ~UNDEVELOPED_WHITE_ROOKS);  // wR
	net_developed_pieces += count_bits(white_DE_pawns & ~bits_between_squares(d2, e2)); // wP (centre pawns)
	net_developed_pieces -= count_bits(black_NBQ & DEVELOPMENT_MASK);					// bN, bB, bQ
	net_developed_pieces -= count_bits(pos->bitboards[bR] & ~UNDEVELOPED_BLACK_ROOKS);  // bR
	net_developed_pieces -= count_bits(black_DE_pawns & ~bits_between_squares(d7, e7)); // bP (centre pawns)

	return ((pos->side == WHITE) ? white_adv : 0) + net_developed_pieces * tempo;
}

// Use the number of squares attack as a proxy for piece activity / mobility
// Based on jk_182's Lichess article: https://lichess.org/@/jk_182/blog/calculating-piece-activity/FAOY6Ii7
static inline int16_t count_activity() {

	int white_activity = 0;
	int black_activity = 0;

	for (int i = 0; i < 32; ++i) {
		if (white_attackers[i] == EMPTY) {
			break; // End of attacks
		}
		white_activity += count_bits(piece_attacks_white[i]) + count_bits(piece_attacks_white[i] & TOP_HALF);
	}
	for (int i = 0; i < 32; ++i) {
		if (black_attackers[i] == EMPTY) {
			break; // End of attacks
		}
		black_activity += count_bits(piece_attacks_black[i]) + count_bits(piece_attacks_black[i] & BOTTOM_HALF);
	}

	return (white_activity - black_activity) * 3 / 2;
}