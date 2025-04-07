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

static inline int negamax_alphabeta(Board* pos, HashTable* table, SearchInfo* info, int alpha, int beta, int depth, bool do_null);

/*
	Iterative deepening loop
*/

void search_position(Board* pos, HashTable* table, SearchInfo* info) {

	int best_score = -INF_BOUND;
	int best_move = NO_MOVE;
	int PV_moves = 0;

	// Aspiration windows variables
	uint8_t window_size = 50; // Size for first 6 depths
	int guess = -INF_BOUND;
	int alpha = -INF_BOUND;
	int beta = INF_BOUND;

	clear_search_vars(pos, table, info); // Initialise searchHistory and killers

	/*
	// Get moves from opening book
	if (EngineOptions->UseBook == TRUE) {
		bestMove = GetBookMove(pos);
	}
	*/

	// No book move available. Find best move via search.
	if (best_move == NO_MOVE) {

		for (int curr_depth = 1; curr_depth <= info->depth; ++curr_depth) {

			/*
				Aspiration windows
			*/

			// Do a full search on depth 1
			if (curr_depth == 1) {
				best_score = negamax_alphabeta(pos, table, info, -INF_BOUND, INF_BOUND, curr_depth, true);
			}
			else {
				if (curr_depth > 6) {
					// Window size decreases linearly with depth, with a minimum value of 25
					window_size = std::max(-2.5 * curr_depth + 65, 25.0);
				}
				alpha = guess - window_size;
				beta = guess + window_size;

				bool reSearch = true;
				while (reSearch) {
					best_score = negamax_alphabeta(pos, table, info, alpha, beta, curr_depth, true);

					// Re-search with a wider window on the side that fails
					if (best_score <= alpha) {
						alpha = -INF_BOUND;
					}
					else if (best_score >= beta) {
						beta = INF_BOUND;
					}
					else {
						// Successful search, exit re-search loop
						reSearch = false;
					}
				}
			}

			guess = best_score;

			PV_moves = get_PV_line(pos, table, curr_depth);
			best_move = pos->get_PV_move(0);

			if (info->stopped) {
				break;
			}

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
					<< " score mate " << mate_moves
					<< " nodes " << info->nodes
					<< " nps " << nps
					<< " hashfull " << (uint32_t)(table->num_entries / double(table->max_entries) * 1000)
					<< " time " << time
					<< " pv";
			}
			else {
				std::cout << "info depth " << curr_depth
					<< " seldepth " << (int)info->seldepth
					<< " score cp " << best_score
					<< " nodes " << info->nodes
					<< " nps " << nps
					<< " hashfull " << (int)(table->num_entries / double(table->max_entries) * 1000)
					<< " time " << time
					<< " pv";
			}

			// Print PV
			for (int i = 0; i < PV_moves; ++i) {
				std::cout << " " << print_move(pos->get_PV_move(i));
			}
			std::cout << "\n";

			// Exit search if mate at current depth is found, in order to save time
			if (mate_found && (curr_depth > (abs(mate_moves) + 1))) {
				break; // Buggy if insufficient search is performed before pruning immediately
			}
		}
	}

	std::cout << "bestmove " << print_move(best_move) << "\n";
}

/*
	Main search components
*/

static inline int quiescence(Board* pos, SearchInfo* info, int alpha, int beta) {

	check_time(info); // Check if time is up

	if (check_repetition(pos) || pos->get_fifty_move() >= 100) {
		return 0;
	}

	if (pos->get_ply() >= MAX_DEPTH) {
		return evaluate_pos(pos);
	}

	if (pos->get_ply() > info->seldepth) {
		info->seldepth = pos->get_ply();
	}

	int stand_pat = evaluate_pos(pos);
	int best_score = stand_pat;

	if (stand_pat >= beta) {
		return stand_pat;
	}
	if (alpha < stand_pat) {
		alpha = stand_pat;
	}
	
	std::vector<Move> list;
	generate_moves(pos, list, true);

	uint16_t legal = 0;

	sort_moves(pos, list);

	for (int move_num = 0; move_num < list.size(); ++move_num) {

		// Check if it's a legal move
		if (!make_move(pos, list.at(move_num).move)) {
			continue;
		}
		info->nodes++;
		legal++;

		int score = -quiescence(pos, info, -beta, -alpha);

		take_move(pos);

		if (info->stopped) {
			return 0;
		}

		if (score >= beta) {
			if (legal == 1) {
				info->fhf++;
			}
			info->fh++;
			return score;
		}
		if (score >= best_score) {
			best_score = score;
		}
		if (score > alpha) {
			alpha = score;
		}
	}

	return best_score;
}

// Negamax Search with Alpha-beta Pruning and Principle Variation Search (PVS)
static inline int negamax_alphabeta(Board* pos, HashTable* table, SearchInfo* info, int alpha, int beta, int depth, bool do_null) {

	if (depth <= 0) {
		return quiescence(pos, info, alpha, beta);
	}

	check_time(info); // Check if time is up

	// Check draw
	if ((check_repetition(pos) || pos->get_fifty_move() >= 100) && pos->get_ply()) {
		return 0;
	}

	// Max depth reached
	if (pos->get_ply() >= MAX_DEPTH) {
		return evaluate_pos(pos);
	}

	if (pos->get_ply() > info->seldepth) {
		info->seldepth = pos->get_ply();
	}

	uint8_t US = pos->get_side();
	uint8_t THEM = pos->get_side() ^ 1;

	// Check extension to avoid horizon effect
	bool in_check = is_square_attacked(pos, pos->get_king_sq(US), THEM);
	if (in_check) {
		depth++;
	}

	int best_score = -INF_BOUND;
	int score = -INF_BOUND;
	int PV_move = NO_MOVE;

	if (probe_hash_entry(pos, table, PV_move, score, alpha, beta, depth)) {
		table->cut++;
		return score;
	}

	/*
		Null-move Pruning
	*/

	// uint8_t big_pieces = count_bits(pos->get_occupancy(US)) - count_bits(pos->get_bitboard((US == WHITE) ? wP : bP));
	bool is_endgame = count_bits(pos->get_occupancy(BOTH)) < 8;
	// Depth thresold and phase check. Null move fails to detect zugzwangs, which are common in endgames.
	if (depth >= 4 && !is_endgame) {
		if (do_null && !in_check && pos->get_ply()) {
			Board* copy = pos->clone();
			make_null_move(copy);
			score = -negamax_alphabeta(copy, table, info, -beta, -beta + 1, depth - 4, false);
			delete copy;

			if (info->stopped) {
				return 0;
			}

			if (score >= beta && abs(score) < MATE_SCORE) {
				info->null_cut++;
				return score;
			}
		}
	}

	std::vector<Move> list;
	generate_moves(pos, list, false);

	int legal = 0;
	int old_alpha = alpha;
	int best_move = NO_MOVE;

	// Order PV move as first move (linear search)
	if (PV_move != NO_MOVE) {
		for (int move_num = 0; move_num < list.size(); ++move_num) {
			if (list.at(move_num).move == PV_move) {
				list.at(move_num).score = 10'000'000;
				break;
			}
		}
	}

	sort_moves(pos, list);

	for (int move_num = 0; move_num < list.size(); ++move_num) {
		int curr_move = list.at(move_num).move;

		int captured = get_move_captured(curr_move);
		bool is_promotion = (bool)get_move_promoted(curr_move);

		// Check if it's a legal move
		// The move will be made for the rest of the code if it is
		if (!make_move(pos, curr_move)) {
			continue;
		}
		legal++;
		info->nodes++;

		/*
			Late Move Reductions
		*/
		// We calculate less promising moves at lower depths

		int reduced_depth = depth - 1; // We move further into the tree

		// Do not reduce if it's completely winning / near mating position
		// Check if it's a "late move"
		if (abs(score) < MATE_SCORE && depth >= 4 && move_num >= 4) {
			uint8_t self_king_sq = pos->get_king_sq(US);
			uint8_t moving_pce = get_move_piece(curr_move);
			uint8_t target_sq = get_move_target(curr_move);
			// uint8_t MoveIsAttack = IsAttack(moving_pce, target_sq, pos);
			// uint8_t target_sq_within_king_zone = dist_between_squares(self_king_sq, target_sq) <= 3; // Checks if a move's target square is within 3 king moves
			
			if (!in_check && captured == 0 && !is_promotion && piece_type[moving_pce] != PAWN) {
				int r = std::max(0, LMR_reduction_table[depth][move_num]); // Depth to be reduced
				reduced_depth = std::max(reduced_depth - r, 1);
			}
		}

		score = -negamax_alphabeta(pos, table, info, -beta, -alpha, reduced_depth, true);
		
		take_move(pos);

		if (info->stopped) {
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

					if (get_move_captured(curr_move) == 0) {
						pos->set_killer_move(1, pos->get_ply(), pos->get_killer_move(0, pos->get_ply()));
						pos->set_killer_move(0, pos->get_ply(), curr_move);
					}

					store_hash_entry(pos, table, best_move, best_score, HFBETA, depth);

					return best_score; // Fail-soft beta-cutoff
				}

				alpha = score;

				if (get_move_captured(curr_move) == 0) {
					int current_score = pos->get_history_score(pos->get_piece(get_move_source(best_move)), get_move_target(best_move));
					pos->set_history_score(pos->get_piece(get_move_source(best_move)), get_move_target(best_move), current_score + depth);
				}
			}
		}
	}

	if (legal == 0) {
		if (in_check) {
			// Checkmate
			return -INF_BOUND + pos->get_ply(); 
		}
		else {
			// Stalemate
			return 0;
		}
	}

	if (alpha != old_alpha) {
		store_hash_entry(pos, table, best_move, best_score, HFEXACT, depth);
	}
	else {
		store_hash_entry(pos, table, best_move, alpha, HFALPHA, depth);
	}

	return best_score;
}

/*
	Helper functions
*/

// Check if the time is up, if there's an interrupt from GUI
static inline void check_time(SearchInfo* info) {
	if (info->timeset && (get_time_ms() > info->stop_time)) {
		info->stopped = true;
	}
}

// Check if there's a two-fold repetition (linear search)
static inline bool check_repetition(const Board* pos) {
	UndoBox* history = pos->get_move_history();
	for (int i = pos->get_his_ply() - pos->get_fifty_move(); i < pos->get_his_ply() - 1; ++i) {
		if (pos->get_hash_key() == history[i].hash_key) {
			free(history);
			return true;
		}
	}
	free(history);
	return false;
}

static inline void clear_search_vars(Board* pos, HashTable* table, SearchInfo* info) {

	for (int pce = 0; pce < 13; ++pce) {
		for (int sq = 0; sq < 64; ++sq) {
			pos->set_history_score(pce, sq, 40000);
		}
	}
	for (int id = 0; id < 2; ++id) {
		for (int depth = 0; depth < MAX_DEPTH; ++depth) {
			pos->set_killer_move(id, depth, 40000);
		}
	}

	table->overwrite = 0;
	table->hit = 0;
	table->cut = 0;
	table->table_age++;
	pos->set_ply(0);
	
	info->seldepth = 0;
	info->stopped = false;
	info->nodes = 0;
	info->fh = 0;
	info->fhf = 0;
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
			LMR_reduction_table[depth][move_num] = int(1 + log(depth) * log(move_num) / 2.75);
		}
	}
}