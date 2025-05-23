// search.cpp

#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>

#include "search.hpp"
#include "attack.hpp"
#include "bitboard.hpp"
#include "Board.hpp"
#include "datatypes.hpp"
#include "evaluate.hpp"
#include "makemove.hpp"
#include "movegen.hpp"
#include "moveio.hpp"
#include "timeman.hpp"
#include "ttable.hpp"

int LMR_reduction_table[MAX_DEPTH][280];

// Function prototypes
static inline void check_time(SearchInfo* info);
static inline bool check_repetition(const Board* pos);
static void clear_search_vars(Board* pos, HashTable* table, SearchInfo* info);

static inline void init_PVLine(PVLine* line);
static inline void movcpy(int* pTarget, const int* pSource, int n);
static inline void update_best_line(Board* pos, PVLine* pv);

static inline int negamax_alphabeta(Board* pos, HashTable* table, SearchInfo* info, int alpha, int beta, int depth, PVLine* line, bool do_null);

/*
	Iterative deepening loop
*/

void search_position(Board* pos, HashTable* table, SearchInfo* info) {

	int best_score = -INF_BOUND;
	int best_move = NO_MOVE;

	// Aspiration windows variables
	uint8_t window_size = 33;
	int guess = -INF_BOUND;
	int alpha = -INF_BOUND;
	int beta = INF_BOUND;

	clear_search_vars(pos, table, info); // Initialise searchHistory and killers

	// No book move available. Find best move via search.
	if (best_move == NO_MOVE) {

		for (int curr_depth = 1; curr_depth <= info->depth; ++curr_depth) {

			PVLine* pv = new PVLine; // Stores the best PV in the search depth so far. Merges with PV of child nodes if it's good
			init_PVLine(pv);

			/*
				Aspiration windows
			*/

			// Do a full search first 3 depths as they are unstable
			if (curr_depth == 3) {
				best_score = negamax_alphabeta(pos, table, info, -INF_BOUND, INF_BOUND, curr_depth, pv, true);
			}
			else {
				alpha = std::max(-INF_BOUND, guess - window_size);
				beta = std::min(guess + window_size, INF_BOUND);
				int delta = window_size;

				// Aspiration windows algorithm adapted from Ethereal by Andrew Grant
				bool reSearch = true;
				while (reSearch) {
					best_score = negamax_alphabeta(pos, table, info, alpha, beta, curr_depth, pv, true);

					// Re-search with a wider window on the side that fails
					if (best_score <= alpha) {
						// Slide the window down
						beta = (alpha + beta) / 2;
						alpha = std::max(-INF_BOUND, alpha - delta);
					}
					else if (best_score >= beta) {
						// Increase beta and not touch alpha
						beta = std::min(beta + delta, INF_BOUND);
					}
					// Search falls within expected bounds
					else {
						reSearch = false;
					}

					delta = delta + delta / 2; // Expand the search window
				}
			}

			update_best_line(pos, pv);
			delete pv;
			guess = best_score;

			if (info->stopped) {
				break;
			}

			// Search exited early as hash move found
			int PV_moves = 0;
			if (info->nodes == 0) {
				PV_moves = get_PV_line(pos, table, curr_depth); // Get PV from TT
			}
			best_move = pos->PV_array.moves[0];

			// Display mate if there's forced mate
			uint64_t time = get_time_ms() - info->start_time; // in ms
			uint64_t nps = (int)((info->nodes / (time + 0.01)) * 1000); // Add 0.01ms to prevent division by zero error

			bool mate_found = false; // Save computation
			int8_t mate_moves = 0;

			if (abs(best_score) >= MATE_SCORE) {
				mate_found = true;
				// copysign(1.0, value) outputs +/- 1.0 depending on the sign of "value" (i.e. sgn(value))
				// Note that /2 is integer division (e.g. 3/2 = 1)
				mate_moves = round((INF_BOUND - abs(best_score) - 1) / 2 + 1) * copysign(1.0, best_score);
				std::cout << "info depth " << curr_depth
					<< " seldepth " << (int)info->seldepth
					<< " score mate " << (int)mate_moves
					<< " nodes " << info->nodes
					<< " nps " << nps
					<< " hashfull " << table->num_entries * 1000 / table->max_entries
					<< " time " << time
					<< " pv";
			}
			else {
				std::cout << "info depth " << curr_depth
					<< " seldepth " << (int)info->seldepth
					<< " score cp " << best_score
					<< " nodes " << info->nodes
					<< " nps " << nps
					<< " hashfull " << table->num_entries * 1000 / table->max_entries
					<< " time " << time
					<< " pv";
			}

			// Print PV
			int limit = 0;
			if (PV_moves == 0) {
				limit = pos->PV_array.length; // PV from search
			}
			else {
				limit = PV_moves; // PV from hash table
			}

			for (int i = 0; i < limit; ++i) {
				std::cout << " " << print_move(pos->PV_array.moves[i]);
			}
			std::cout << "\n" << std::flush; // Make sure it outputs depth-by-depth to GUI

			// Second check to make sure it doesn't hang in low time and flag
			check_time(info);
			if (info->stopped) {
				break;
			}

			// Exit search if mate at current depth is found, in order to save time
			if (mate_found && (curr_depth > (abs(mate_moves) + 1))) {
				break; // Buggy if insufficient search is performed before pruning
			}
		}
	}

	std::cout << "bestmove " << print_move(best_move) << "\n" << std::flush;
}

/*
	Main search components
*/

// Quiescence search
static inline int quiescence(Board* pos, SearchInfo* info, int alpha, int beta, PVLine* line) {

	check_time(info); // Check if time is up

	if (check_repetition(pos) || pos->fifty_move >= 100) {
		return 0;
	}

	if (pos->ply >= MAX_DEPTH) {
		return evaluate_pos(pos);
	}

	if (pos->ply > info->seldepth) {
		info->seldepth = pos->ply;
	}

	PVLine* candidate_PV = new PVLine;
	init_PVLine(candidate_PV);

	int stand_pat = evaluate_pos(pos);
	int best_score = stand_pat;
	int score = stand_pat;

	if (stand_pat >= beta) {
		delete candidate_PV;
		return beta;
	}

	if (stand_pat > alpha) {
		alpha = stand_pat;
	}
	
	/*
		Delta pruning (dead lost scenario)
	*/
	constexpr uint16_t big_delta = 936; // Queen eg value
	if (stand_pat + big_delta < alpha) {
		delete candidate_PV;
		return alpha; // We are dead lost, no point searching for improvements
	}

	std::vector<Move> list;
	generate_moves(pos, list, true);

	uint16_t legal = 0;

	sort_moves(pos, list);

	for (int move_num = 0; move_num < (int)list.size(); ++move_num) {

		int curr_move = list[move_num].move;

		/*
		// Delta pruning (general case)
		int captured = get_move_captured(curr_move);
		int promoted = get_move_promoted(curr_move);
		int delta1 = piece_value_MG[captured];
		int delta2 = piece_value_MG[promoted];
		constexpr uint16_t delta_margin = 300;
		bool is_late_endgame = count_bits(pos->occupancies[BOTH]) <= 8;
		// We skip the move if the gain from capturing or promoting a piece is too low
		if (!is_late_endgame && delta1 + delta_margin <= alpha && delta2 + delta_margin <= alpha) {
			delete candidate_PV;
			return alpha;
		}
		*/

		// Check if it's a legal move
		if (!make_move(pos, curr_move)) {
			continue;
		}
		info->nodes++;
		legal++;

		score = -quiescence(pos, info, -beta, -alpha, line);

		take_move(pos);

		if (info->stopped) {
			delete candidate_PV;
			return 0;
		}
		
		if (score > best_score) {
			best_score = score;
		}
		if (score > alpha) {
			if (score >= beta) {
				if (legal == 1) {
					info->fhf++;
				}
				info->fh++;
				delete candidate_PV;
				return score;
			}

			alpha = score;

			// Copy child's PV and prepend the current move
			line->score = score;
			line->length = 1 + candidate_PV->length;
			line->moves[0] = curr_move;
			movcpy(line->moves + 1, candidate_PV->moves, candidate_PV->length);
		}
	}

	delete candidate_PV;
	return best_score;
}

// Negamax Search with Alpha-beta Pruning
static inline int negamax_alphabeta(Board* pos, HashTable* table, SearchInfo* info, int alpha, int beta, int depth, PVLine* line, bool do_null) {

	if (depth <= 0) {
		return quiescence(pos, info, alpha, beta, line);
	}

	check_time(info); // Check if time is up

	// Check draw
	if ((check_repetition(pos) || pos->fifty_move >= 100) && pos->ply) {
		return 0;
	}

	// Max depth reached
	if (pos->ply >= MAX_DEPTH) {
		return evaluate_pos(pos);
	}

	if (pos->ply > info->seldepth) {
		info->seldepth = pos->ply;
	}

	PVLine* candidate_PV = new PVLine;
	init_PVLine(candidate_PV);

	uint8_t US = pos->side;
	uint8_t THEM = US ^ 1;

	// Check extension to avoid horizon effect
	bool in_check = is_square_attacked(pos, pos->king_sq[US], THEM);
	if (in_check) {
		depth++;
	}

	int score = -INF_BOUND;
	int PV_move = NO_MOVE;

	if (probe_hash_entry(pos, table, PV_move, score, alpha, beta, depth)) {
		table->cut++;
		delete candidate_PV;
		return score;
	}

	/*
		Null-move Pruning
	*/

	uint8_t big_pieces = count_bits(pos->occupancies[US] ^ pos->bitboards[(US == WHITE) ? wP : bP]);
	// Depth thresold and phase check. Null move fails to detect zugzwangs, which are common in endgames.
	if (depth >= 4) {
		if (do_null && !in_check && big_pieces > 1 && pos->ply) {
			make_null_move(pos);
			score = -negamax_alphabeta(pos, table, info, -beta, -beta + 1, depth - 4, candidate_PV, false);
			take_null_move(pos);

			if (info->stopped) {
				delete candidate_PV;
				return 0;
			}

			if (score >= beta && abs(score) < MATE_SCORE) {
				info->null_cut++;
				delete candidate_PV;
				return score;
			}
		}
	}

	std::vector<Move> list;
	generate_moves(pos, list, false);

	int legal = 0;
	int old_alpha = alpha;
	int best_move = NO_MOVE;
	int best_score = -INF_BOUND;

	// Order PV move as first move (linear search)
	if (PV_move != NO_MOVE) {
		for (int move_num = 0; move_num < (int)list.size(); ++move_num) {
			if (list.at(move_num).move == PV_move) {
				list.at(move_num).score += 10'000'000;
				break;
			}
		}
	}

	sort_moves(pos, list);

	for (int move_num = 0; move_num < (int)list.size(); ++move_num) {
		int curr_move = list.at(move_num).move;

		bool is_capture = (bool)get_move_captured(curr_move);
		bool is_promotion = (bool)get_move_promoted(curr_move);
		bool is_quiet = !is_capture && !is_promotion;

		// Check if it's a legal move
		// The move will be made for the rest of the code if it is
		if (!make_move(pos, curr_move)) {
			continue;
		}
		legal++;
		info->nodes++;

		/*
			Futility pruning
		*/

		// Don't skip PV move, captures and killers
		if (depth <= 2 && move_num >= 4 && is_quiet && !in_check && abs(score) < MATE_SCORE) {
			int futility_margin = 150 * depth; // Scale margin with depth
			int static_eval = evaluate_pos(pos);

			if (static_eval + futility_margin <= alpha) {
				take_move(pos);
				continue; // Discard moves with no potential to raise alpha
			}
		}

		/*
			Late Move Reductions
		*/
		// We calculate less promising moves at lower depths

		int reduced_depth = depth - 1; // We move further into the tree

		// Do not reduce if it's completely winning / near mating position
		// Check if it's a "late move"
		if (depth >= 4 && move_num >= 4 && abs(score) < MATE_SCORE) {
			uint8_t moving_pce = get_move_piece(curr_move);

			if (is_quiet && !in_check && piece_type[moving_pce] != PAWN) {
				int r = std::max(0, LMR_reduction_table[depth][move_num]); // Depth to be reduced
				reduced_depth = std::max(reduced_depth - r - 1, 1);
			}
		}

		score = -negamax_alphabeta(pos, table, info, -beta, -alpha, reduced_depth, candidate_PV, true);

		// Re-search with full depth if it beats alpha (make sure it's not a fluke)
		if (score > alpha) {
			score = -negamax_alphabeta(pos, table, info, -beta, -alpha, depth - 1, candidate_PV, true);
		}
		
		take_move(pos);

		if (info->stopped) {
			delete candidate_PV;
			return 0;
		}

		// Update best_score and best_move
		if (score > best_score) {
			best_score = score;
			best_move = curr_move;

			if (score > alpha) {

				if (score >= beta) {
					if (legal == 1) {
						info->fhf++;
					}
					info->fh++;

					// If the move that caused the beta cutoff is quiet we have a killer move
					if (!is_capture) {
						pos->killer_moves[1][pos->ply] = pos->killer_moves[0][pos->ply];
						pos->killer_moves[0][pos->ply] = curr_move;
					}

					store_hash_entry(pos, table, best_move, beta, HFBETA, depth);

					delete candidate_PV;
					return beta; // Fail-hard beta-cutoff
				}

				alpha = score;

				// Copy child's PV and prepend the current move
				line->score = score;
				line->length = 1 + candidate_PV->length;
				line->moves[0] = curr_move;
				movcpy(line->moves + 1, candidate_PV->moves, candidate_PV->length);

				// Store the move that beats alpha if it's quiet
				if (!is_capture) {
					pos->history_moves[pos->pieces[get_move_source(best_move)]][get_move_target(best_move)] += depth;
				}
			}
		}
	}

	if (legal == 0) {
		if (in_check) {
			// Checkmate
			delete candidate_PV;
			return -INF_BOUND + pos->ply; 
		}
		else {
			// Stalemate
			delete candidate_PV;
			return 0;
		}
	}

	if (alpha != old_alpha) {
		store_hash_entry(pos, table, best_move, best_score, HFEXACT, depth);
	}
	else {
		store_hash_entry(pos, table, best_move, alpha, HFALPHA, depth);
	}

	delete candidate_PV;
	return alpha;
}

/*
	Helper functions
*/

// Check if the time is up
static inline void check_time(SearchInfo* info) {
	// Check if time is up
	if (info->timeset && (get_time_ms() > info->stop_time)) {
		info->stopped = true;
	}
}

// Check if there's a two-fold repetition (linear search)
static inline bool check_repetition(const Board* pos) {
	const int start = std::max(pos->his_ply - pos->fifty_move, 0);
	const int end = std::min(pos->his_ply - 1, MAX_GAME_MOVES - 1);
	for (int i = start; i <= end; ++i) {
		if (pos->hash_key == pos->move_history[i].hash_key) {
			return true;
		}
	}
	return false;
}

static inline void clear_search_vars(Board* pos, HashTable* table, SearchInfo* info) {

	for (int pce = 0; pce < 13; ++pce) {
		for (int sq = 0; sq < 64; ++sq) {
			pos->history_moves[pce][sq] = 40000;
		}
	}
	for (int id = 0; id < 2; ++id) {
		for (int depth = 0; depth < MAX_DEPTH; ++depth) {
			pos->killer_moves[id][depth] = 40000;
		}
	}

	// Clear PV table
	for (int i = 0; i < MAX_DEPTH; ++i) {
		pos->PV_array.moves[i] = 0; // NO_MOVE
	}

	table->overwrite = 0;
	table->hit = 0;
	table->cut = 0;
	table->table_age++;
	pos->ply = 0;
	
	info->seldepth = 0;
	info->stopped = false;
	info->nodes = 0;
	info->fh = 0.0;
	info->fhf = 0.0;
}

void init_searchinfo(SearchInfo* info) {
	info->start_time = 0;
	info->stop_time = 0;
	info->depth = 0;
	info->seldepth = 0;
	info->nodes = 0;

	info->timeset = false;
	info->movestogo = 0;
	info->quit = false;
	info->stopped = false;

	info->fh = 0.0f;
	info->fhf = 0.0f;
	info->null_cut = 0;
}

void init_LMR_table() {
	for (int depth = 3; depth < MAX_DEPTH; ++depth) {
		for (int move_num = 4; move_num < 280; ++move_num) {
			LMR_reduction_table[depth][move_num] = int(0.25 + log(depth) * log(move_num) / 2.25);
		}
	}
}

/*
	PV management
*/

static inline void init_PVLine(PVLine* line) {
	line->length = 0;
	line->score = -INF_BOUND;
	for (int i = 0; i < MAX_DEPTH; ++i) {
		line->moves[i] = NO_MOVE;
	}
}

// Copies the moves from pSource to pTarget, given the number of moves n
static inline void movcpy(int* pTarget, const int* pSource, int n) {
	for (int i = 0; i < n; ++i) {
		pTarget[i] = pSource[i];
	}
}

static inline void update_best_line(Board* pos, PVLine* pv) {
	if (pv->score > pos->PV_array.score) {
		pos->PV_array.length = pv->length;
		movcpy(pos->PV_array.moves, pv->moves, pv->length);
	}
}