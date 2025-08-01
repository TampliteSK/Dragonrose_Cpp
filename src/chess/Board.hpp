// Board.hpp

#ifndef BOARD_HPP
#define BOARD_HPP

#include "../datatypes.hpp"
#include <string>
#include <cstdint>

typedef unsigned long long Bitboard;

#define GET_RANK(sq) ((sq) / 8)
#define GET_FILE(sq) ((sq) % 8)
#define FR2SQ(f, r) ((r) * 8 + (f))

#define START_POS "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

typedef struct {
	int move;
	int castle_perms;
	int enpas;
	int fifty_move;
	uint64_t hash_key;
} UndoBox;

typedef struct {
	uint8_t pieces[64]; // Square -> Piece
	Bitboard bitboards[13];
	Bitboard occupancies[3];

	uint8_t piece_num[13];
	uint8_t king_sq[3];
	uint8_t side;
	uint8_t enpas;
	uint8_t castle_perms;
	uint8_t fifty_move; // Counter for 50-move rule

	uint8_t ply;
	uint8_t his_ply;
	uint64_t hash_key;
	UndoBox move_history[2048]; // Fixed indices, easier to manage than vector

	int killer_moves[2][64]; // killer moves [id][ply]
	int history_moves[13][64]; // history moves [piece][square]
	PVLine PV_array; // Stores the final best PV after every depth
} Board;

// Board functions
void reset_board(Board* pos);
void update_vars(Board* pos);
void parse_fen(Board* pos, const std::string FEN);

void print_board(const Board* pos);
void print_move_history(const Board* pos);
bool check_boards(const Board* pos1, const Board* pos2);

#endif // BOARD_HPP