// makemove.hpp

#include <iostream>
#include <cstdint>
#include "makemove.hpp"
#include "Board.hpp"
#include "zobrist.hpp"
#include "bitboard.hpp"
#include "movegen.hpp"
#include "attack.hpp"

// Functions based on VICE makemove.c by Richard Allbert
// Incrementally updating Board class move by move for better efficiency

/*
	Hash helper functions
*/

static inline void HASH_PCE(Board *pos, uint8_t pce, uint8_t sq) {
	uint64_t key = pos->get_hash_key();
	key ^= piece_keys[pce][sq];
	pos->set_hash_key(key);
}
static inline void HASH_CA(Board *pos) {
	uint64_t key = pos->get_hash_key();
	key ^= castle_keys[pos->get_castle_perms()];
	pos->set_hash_key(key);
}
static inline void HASH_SIDE(Board *pos) {
	uint64_t key = pos->get_hash_key();
	key ^= side_key;
	pos->set_hash_key(key);
}
static inline void HASH_EP(Board *pos) {
	uint64_t key = pos->get_hash_key();
	key ^= piece_keys[EMPTY][pos->get_enpas()];
	pos->set_hash_key(key);
}

/*
	Piece manipulation
*/

static inline void clear_piece(Board *pos, const int sq) {
	int pce = pos->get_piece(sq);
	int col = piece_col[pce];

	HASH_PCE(pos, pce, sq);

	pos->set_piece(sq, EMPTY);

	pos->set_bitboard(pce, sq, 0);
	pos->set_occupancy(col, sq, 0);
	pos->set_occupancy(BOTH, sq, 0);

	pos->set_piece_num(pce, pos->get_piece_num(pce) - 1);
}

static void add_piece(Board *pos, const int sq, const int pce) {

	int col = piece_col[pce];
	HASH_PCE(pos, pce, sq);

	pos->set_piece(sq, pce);

	pos->set_bitboard(pce, sq, 1);
	pos->set_occupancy(col, sq, 1);
	pos->set_occupancy(BOTH, sq, 1);

	pos->set_piece_num(pce, pos->get_piece_num(pce) + 1);
}

static void move_piece(Board *pos, const int from, const int to) {
	int pce = pos->get_piece(from);
	int col = piece_col[pce];

	HASH_PCE(pos, pce, from);
	pos->set_piece(from, EMPTY);
	pos->set_bitboard(pce, from, 0);
	pos->set_occupancy(col, from, 0);
	pos->set_occupancy(BOTH, from, 0);

	HASH_PCE(pos, pce, to);
	pos->set_piece(to, pce);
	pos->set_bitboard(pce, to, 1);
	pos->set_occupancy(col, to, 1);
	pos->set_occupancy(BOTH, to, 1);
}

/*
	Move manipulation
*/

void take_move(Board *pos) {
	pos->set_ply(pos->get_ply() - 1);
	pos->set_his_ply(pos->get_his_ply() - 1);

	UndoBox* history = pos->get_move_history();
	UndoBox box = history[pos->get_his_ply()];
	int move = box.move;
	int from = get_move_source(move);
	int to = get_move_target(move);

	// Unhash enpasant castling
	if (pos->get_enpas() != NO_SQ) {
		HASH_EP(pos);
	}
	HASH_CA(pos);

	pos->set_castle_perms(box.castle_perms);
	pos->set_fifty_move(box.fifty_move);
	pos->set_enpas(box.enpas);
	delete history;

	// Rehash
	if (pos->get_enpas() != NO_SQ) {
		HASH_EP(pos);
	}
	HASH_CA(pos);

	pos->set_side(pos->get_side() ^ 1);
	HASH_SIDE(pos);

	if (get_move_enpassant(move)) {
		// Recover the pawn that was enpassanted
		if (pos->get_side() == WHITE) {
			add_piece(pos, to + 8, bP);
		}
		else {
			add_piece(pos, to - 8, wP);
		}
	}
	else if (get_move_castling(move)) {
		// Move the castling rook back
		switch (to) {
			case c1: move_piece(pos, d1, a1); break;
			case c8: move_piece(pos, d8, a8); break;
			case g1: move_piece(pos, f1, h1); break;
			case g8: move_piece(pos, f8, h8); break;
		}
	}

	move_piece(pos, to, from);

	if (piece_type[pos->get_piece(from)] == KING) {
		pos->set_king_sq(pos->get_side(), from);
	}

	// Undo captures
	int captured = get_move_capture(move);
	if (captured != EMPTY) {
		add_piece(pos, to, captured);
	}

	// Undo promotion
	if (get_move_promoted(move) != EMPTY) {
		clear_piece(pos, to);
		add_piece(pos, from, (piece_col[get_move_promoted(move)] == WHITE) ? wP : bP);
	}
}

bool make_move(Board* pos, int move) {
	int from = get_move_source(move);
	int to = get_move_target(move);
	int side = pos->get_side();

	UndoBox box = { 0, 0, 0, 0, 0 };
	box.hash_key = pos->get_hash_key();

	if (get_move_enpassant(move)) {
		// Clear the captured pawn
		if (side == WHITE) {
			clear_piece(pos, to + 8);
		}
		else {
			clear_piece(pos, to - 8);
		}
	}
	else if (get_move_castling(move)) {
		// Move the corresponding rook
		switch (to) {
		case c1:
			move_piece(pos, a1, d1);
			break;
		case c8:
			move_piece(pos, a8, d8);
			break;
		case g1:
			move_piece(pos, h1, f1);
			break;
		case g8:
			move_piece(pos, h8, f8);
			break;
		}
	}

	if (pos->get_enpas() != NO_SQ) {
		HASH_EP(pos);
	}
	HASH_CA(pos);

	box.move = move;
	box.fifty_move = pos->get_fifty_move();
	box.enpas = pos->get_enpas();
	box.castle_perms = pos->get_castle_perms();
	pos->set_move_history(pos->get_his_ply(), box);

	uint8_t castle_perms = pos->get_castle_perms();
	castle_perms &= castling_rights[from];
	castle_perms &= castling_rights[to];
	pos->set_castle_perms(castle_perms);
	pos->set_enpas(NO_SQ);

	HASH_CA(pos);

	pos->set_fifty_move(pos->get_fifty_move() + 1);

	// Handle captures
	int captured = get_move_capture(move);
	if (captured != EMPTY) {
		clear_piece(pos, to);
		pos->set_fifty_move(0);
	}


	pos->set_ply(pos->get_ply() + 1);
	pos->set_his_ply(pos->get_his_ply() + 1);

	if (piece_type[pos->get_piece(from)] == PAWN) {
		pos->set_fifty_move(0);
		// Check if it's double advance
		if (get_move_double(move)) {
			if (side == WHITE) {
				pos->set_enpas(from - 8);
			}
			else {
				pos->set_enpas(from + 8);
			}
			HASH_EP(pos);
		}
	}

	move_piece(pos, from, to);

	// Handle promotions
	int prPce = get_move_promoted(move);
	if (prPce != EMPTY) {
		// Replace the pawn with the promoted piece
		clear_piece(pos, to);
		add_piece(pos, to, prPce);
	}

	// Detect king move
	if (piece_type[pos->get_piece(to)] == KING) {
		pos->set_king_sq(pos->get_side(), to);
	}

	// Switch sides
	pos->set_side(pos->get_side() ^ 1);
	HASH_SIDE(pos);

	// It is an illegal move if we reveal a check to our king
	// side: Our side (the mover)
	// pos->get_side(): Opponent's side (switched after making the move)
	if (is_square_attacked(pos, pos->get_king_sq(side), pos->get_side())) {
		take_move(pos);
		return false; // Illegal move
	}

	return true;
}

/*
bool make_move(Board *pos, int move) {

	Board* copy = pos->clone();

	int from = get_move_source(move);
	int to = get_move_target(move);
	int side = pos->get_side();
	
	UndoBox box = {0, 0, 0, 0, 0};
	box.hash_key = pos->get_hash_key();

	if (get_move_enpassant(move)) {
		// Clear the captured pawn
		if (side == WHITE) {
			clear_piece(copy, to + 8);
		}
		else {
			clear_piece(copy, to - 8);
		}
	}
	else if (get_move_castling(move)) {
		// Move the corresponding rook
		switch (to) {
			case c1:
				move_piece(copy, a1, d1);
				break;
			case c8:
				move_piece(copy, a8, d8);
				break;
			case g1:
				move_piece(copy, h1, f1);
				break;
			case g8:
				move_piece(copy, h8, f8);
				break;
		}
	}

	if (copy->get_enpas() != NO_SQ) {
		HASH_EP(copy);
	}
	HASH_CA(copy);

	box.move = move;
	box.fifty_move = copy->get_fifty_move();
	box.enpas = copy->get_enpas();
	box.castle_perms = copy->get_castle_perms();
	copy->set_move_history(copy->get_his_ply(), box);

	uint8_t castle_perms = copy->get_castle_perms();
	castle_perms &= castling_rights[from];
	castle_perms &= castling_rights[to];
	copy->set_castle_perms(castle_perms);
	copy->set_enpas(NO_SQ);

	HASH_CA(copy);

	copy->set_fifty_move(copy->get_fifty_move() + 1);

	// Handle captures
	int captured = get_move_capture(move);
	if (captured != EMPTY) {
		clear_piece(copy, to);
		copy->set_fifty_move(0);
	}


	copy->set_ply(copy->get_ply() + 1);
	copy->set_his_ply(copy->get_his_ply() + 1);

	if (piece_type[copy->get_piece(from)] == PAWN) {
		copy->set_fifty_move(0);
		// Check if it's double advance
		if (get_move_double(move)) {
			if (side == WHITE) {
				copy->set_enpas(from - 8);
			}
			else {
				copy->set_enpas(from + 8);
			}
			HASH_EP(copy);
		}
	}

	move_piece(copy, from, to);

	// Handle promotions
	int prPce = get_move_promoted(move);
	if (prPce != EMPTY) {
		// Replace the pawn with the promoted piece
		clear_piece(copy, to);
		add_piece(copy, to, prPce);
	}

	// Detect king move
	if (piece_type[copy->get_piece(to)] == KING) {
		copy->set_king_sq(copy->get_side(), to);
	}
	
	// Switch sides
	copy->set_side(copy->get_side() ^ 1);
	HASH_SIDE(copy);

	// It is an illegal move if we reveal a check to our king
	// side: Our side (the mover)
	// pos->get_side(): Opponent's side (switched after making the move)
	if (is_square_attacked(copy, copy->get_king_sq(side), copy->get_side())) {
		delete copy;
		return false; // Illegal move
	}

	*pos = *copy;
	delete copy;
	return true;
}
*/

/*
	Null move manipulation
*/

void make_null_move(Board *pos) {

	pos->set_ply(pos->get_ply() + 1);
	UndoBox* history = pos->get_move_history();
	UndoBox box = history[pos->get_his_ply()];
	box.hash_key = pos->get_hash_key();

	if (pos->get_enpas() != NO_SQ) {
		HASH_EP(pos);
	}

	box.move = NO_MOVE;
	box.fifty_move = pos->get_fifty_move();
	box.enpas = pos->get_enpas();
	box.castle_perms = pos->get_castle_perms();
	free(history);

	pos->set_enpas(NO_SQ);
	pos->set_side(pos->get_side() ^ 1);
	pos->set_his_ply(pos->get_his_ply() + 1);
	HASH_SIDE(pos);

	return;
}

void take_null_move(Board *pos) {

	pos->set_his_ply(pos->get_his_ply() - 1);
	pos->set_ply(pos->get_ply() - 1);

	if (pos->get_enpas() != NO_SQ) {
		HASH_EP(pos);
	}

	UndoBox* history = pos->get_move_history();
	UndoBox box = history[pos->get_his_ply()];

	pos->set_castle_perms(box.castle_perms);
	pos->set_fifty_move(box.fifty_move);
	pos->set_enpas(box.enpas);

	if (pos->get_enpas() != NO_SQ) {
		HASH_EP(pos);
	}
	pos->set_side(pos->get_side() ^ 1);
	HASH_SIDE(pos);
}