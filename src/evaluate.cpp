// evaluate.cpp

#include <algorithm>
#include "evaluate.hpp"
#include "datatypes.hpp"
#include "attack.hpp"
#include "Board.hpp"
#include "bitboard.hpp"
#include "endgame.hpp"

// PesTO Material
uint16_t piece_value_MG[13] = { 0, 82, 337, 365, 477, 1025, 30000, 82, 337, 365, 477, 1025, 30000 };
uint16_t piece_value_EG[13] = { 0, 94, 281, 297, 512,  936, 30000, 94, 281, 297, 512,  936, 30000 };

// Function prototypes
static inline double MG_weight(const Board* pos);

static inline double evaluate_pawns(const Board* pos, uint8_t pce, double weight); 
static inline double evaluate_knights(const Board* pos, uint8_t pce, double weight);
static inline double evaluate_bishops(const Board* pos, uint8_t pce, double weight);
static inline double evaluate_rooks(const Board* pos, uint8_t pce, double weight);
static inline double evaluate_queens(const Board* pos, uint8_t pce, double weight);
static inline double evaluate_kings(const Board* pos, uint8_t pce, double weight);

static inline int16_t count_tempi(const Board* pos);
static inline int8_t count_activity(const Board* pos);

int evaluate_pos(const Board* pos) {

	Bitboard pawns = pos->get_bitboard(wP) | pos->get_bitboard(bP);
	bool is_TB_endgame = count_bits(pos->get_occupancy(BOTH)) - count_bits(pawns) < 8;
	if (is_TB_endgame && count_bits(pawns) == 0) {
		if (check_material_draw(pos)) {
			return endgame_noise(3);
		}
	}

	bool is_endgame = count_bits(pos->get_occupancy(BOTH)) < 8;
	double score = 0;
	double weight = MG_weight(pos);

	score += count_material(pos);

	for (int colour = WHITE; colour <= BLACK; ++colour) {
		int8_t sign = (colour == WHITE) ? 1 : -1;
		uint8_t col_offset = (colour == WHITE) ? 0 : 6;
		score += evaluate_pawns(pos, wP + col_offset, weight) * sign;
		score += evaluate_knights(pos, wN + col_offset, weight) * sign;
		score += evaluate_bishops(pos, wB + col_offset, weight) * sign;
		score += evaluate_rooks(pos, wR + col_offset, weight) * sign;
		score += evaluate_queens(pos, wQ + col_offset, weight) * sign;
		score += evaluate_kings(pos, wK + col_offset, weight) * sign;
	}

	// Tempi
	// score += count_tempi(pos) * MG_weight(pos); // Towards endgame considering tempi is useless

	// Piece activity
	score += count_activity(pos);
	
	// Bishop pair bonus
	if (pos->get_piece_num(wB) > 2) score += bishop_pair;
	if (pos->get_piece_num(bB) > 2) score -= bishop_pair;
	
	// 50-move rule adjustment
	if (score < MATE_SCORE) {
		score = (score * ((100 - pos->get_fifty_move()) / 100.0));
	}

	// Endgame noise adjustment
	if (is_endgame) {
		score += /*check_material_win(pos) +*/ endgame_noise(3);
	}

	// Perspective adjustment
	return (int)(score * ((pos->get_side() == WHITE) ? 1 : -1));
}

/*
	Components
*/

// Calculates the middlegame weight for tapered eval.
static inline double MG_weight(const Board* pos) {

	// Applying game_phase at startpos
	#define opening_phase 64

	// Caissa game phase formula (0.11e)
	// Performs about 200 elo better than PesTO's own tapered eval and directly scaling to material
	uint8_t game_phase = 3 * (pos->get_piece_num(wN) + pos->get_piece_num(bN) + pos->get_piece_num(wB) + pos->get_piece_num(bB));
	game_phase += 5 * (pos->get_piece_num(wR) + pos->get_piece_num(bR));
	game_phase += 10 * (pos->get_piece_num(wQ) + pos->get_piece_num(bQ));
	game_phase = std::min((int)game_phase, opening_phase); // Capped in case of early promotion

	return game_phase / (double)opening_phase;
}

// Calculates the material from White's perspective
int count_material(const Board* pos) {
	double sum = 0;
	double weight = MG_weight(pos);

	for (int pce = wP; pce <= bK; ++pce) {
		double value = pos->get_piece_num(pce) * ( piece_value_MG[pce] * weight + piece_value_EG[pce] * (1 - weight) );
		sum += value * ((piece_col[pce] == WHITE) ? 1 : -1);
	}

	return (int)sum;
}

static inline double compute_PSQT(uint8_t pce, uint8_t sq, double weight) {
	double score = 0;
	uint8_t col = piece_col[pce];
	if (col == WHITE) {
		score += PSQT_MG[piece_type[pce] - 1][sq] * weight + PSQT_EG[piece_type[pce] - 1][sq] * (1 - weight);
	}
	else {
		score += PSQT_MG[piece_type[pce] - 1][Mirror64[sq]] * weight + PSQT_EG[piece_type[pce] - 1][Mirror64[sq]] * (1 - weight);
	}
	return score;
}

/*
	Piece evaluation
*/

static inline double evaluate_pawns(const Board* pos, uint8_t pce, double weight) {

	double score = 0;
	Bitboard pawns = pos->get_bitboard(pce);

	while (pawns) {
		uint8_t sq = pop_ls1b(pawns);
		score += compute_PSQT(pce, sq, weight);
	}

	return score;
}

static inline double evaluate_knights(const Board* pos, uint8_t pce, double weight) {

	double score = 0;
	Bitboard knights = pos->get_bitboard(pce);

	while (knights) {
		uint8_t sq = pop_ls1b(knights);
		score += compute_PSQT(pce, sq, weight);
	}

	return score;
}

static inline double evaluate_bishops(const Board* pos, uint8_t pce, double weight) {

	double score = 0;
	Bitboard bishops = pos->get_bitboard(pce);

	while (bishops) {
		uint8_t sq = pop_ls1b(bishops);
		score += compute_PSQT(pce, sq, weight);
	}

	return score;
}

static inline double evaluate_rooks(const Board* pos, uint8_t pce, double weight) {

	double score = 0;
	Bitboard rooks = pos->get_bitboard(pce);

	while (rooks) {
		uint8_t sq = pop_ls1b(rooks);
		score += compute_PSQT(pce, sq, weight);
	}

	return score;
}

static inline double evaluate_queens(const Board* pos, uint8_t pce, double weight) {

	double score = 0;
	Bitboard queens = pos->get_bitboard(pce);

	while (queens) {
		uint8_t sq = pop_ls1b(queens);
		score += compute_PSQT(pce, sq, weight);
	}

	return score;
}

static inline double evaluate_kings(const Board* pos, uint8_t pce, double weight) {

	double score = 0;
	
	uint8_t sq = pos->get_king_sq(piece_col[pce]);
	score += compute_PSQT(pce, sq, weight);

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
	uint8_t tempo = 22; // Base value of a tempo
	Bitboard UNDEVELOPED_WHITE_ROOKS = (1ULL << a1) | (1ULL << h1);
	Bitboard UNDEVELOPED_BLACK_ROOKS = (1ULL << a8) | (1ULL << h8);

	int8_t net_developed_pieces_BB = 0;
	net_developed_pieces_BB += count_bits((pos->get_occupancy(WHITE) ^ pos->get_bitboard(wP)) & DEVELOPMENT_MASK); // wN, wB, wQ
	net_developed_pieces_BB += count_bits( ~(pos->get_bitboard(wR) & UNDEVELOPED_WHITE_ROOKS) );               // wR
	net_developed_pieces_BB += count_bits( ~(pos->get_bitboard(wP) & bits_between_squares(d2, e2)) );          // wP (centre pawns)
	net_developed_pieces_BB -= count_bits((pos->get_occupancy(BLACK) ^ pos->get_bitboard(bP)) & DEVELOPMENT_MASK); // bN, bB, b
	net_developed_pieces_BB -= count_bits( ~(pos->get_bitboard(bR) & UNDEVELOPED_BLACK_ROOKS) );               // bR
	net_developed_pieces_BB -= count_bits( ~(pos->get_bitboard(bP) & bits_between_squares(d7, e7)) );          // bP (centre pawns)

	// Add 1 tempo to account for White's first-move advantage
	return (1 + net_developed_pieces_BB) * tempo;
}

// Use the number of squares attack as a proxy for piece activity / mobility
// Based on jk_182's Lichess article: https://lichess.org/@/jk_182/blog/calculating-piece-activity/FAOY6Ii7
static inline int8_t count_activity(const Board* pos) {
	Bitboard white_attacks = get_all_attacks(pos, WHITE, false);
	Bitboard black_attacks = get_all_attacks(pos, BLACK, false);

	// Double count attacks in the enemy's half
	int8_t net_white_activity = count_bits(white_attacks) + count_bits(white_attacks & TOP_HALF)
						      - count_bits(black_attacks) - count_bits(black_attacks & BOTTOM_HALF);
	return net_white_activity * 2;
}