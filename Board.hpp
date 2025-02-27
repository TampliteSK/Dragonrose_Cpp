// Board.hpp

#ifndef BOARD_HPP
#define BOARD_HPP

#include <string>
#include <cstdint>
#include "datatypes.hpp"
#include "search.hpp"

const uint16_t MAX_GAME_MOVES = 2048;

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

class Board {
public:
	Board();
	Board(const std::string FEN);
	void reset_board();
	void update_vars();
	void parse_fen(const std::string FEN);
	void print_board() const;

	// Getters
	uint8_t get_piece(uint8_t sq) const;
	Bitboard get_bitboard(uint8_t pce) const;
	Bitboard get_occupancy(uint8_t col) const;

	uint8_t get_piece_num(uint8_t pce) const;

	uint8_t get_king_sq(uint8_t col) const;
	uint8_t get_side() const;
	uint8_t get_enpas() const;
	uint8_t get_castle_perms() const;
	uint8_t get_fifty_move() const;

	uint8_t get_ply() const;
	uint16_t get_his_ply() const;
	uint64_t get_hash_key() const;
	UndoBox* get_move_history() const;

	int get_killer_move(uint8_t id, uint8_t depth) const;
	int get_history_move(uint8_t pce, uint8_t sq) const;

	// Setters
	void set_piece(uint8_t sq, uint8_t new_pce);
	void set_bitboard(uint8_t pce, uint8_t index, uint8_t new_bit);
	void set_occupancy(uint8_t col, uint8_t index, uint8_t new_bit);
	
	void set_piece_num(uint8_t pce, uint8_t new_num);

	void set_king_sq(uint8_t col, uint8_t new_sq);
	void set_side(uint8_t new_side);
	void set_enpas(uint8_t new_sq);
	void set_castle_perms(uint8_t new_perms);
	void set_fifty_move(uint8_t new_count);

	void set_ply(uint8_t new_ply);
	void set_his_ply(uint16_t new_his_ply);
	void set_hash_key(uint64_t new_key);
	void set_move_history(int index, UndoBox new_box);

	void set_killer_move(uint8_t id, uint8_t depth, int new_move);
	void set_history_move(uint8_t pce, uint8_t sq, int new_move);

	bool operator==(const Board& other) const;
private:
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
	uint16_t his_ply;
	uint64_t hash_key;
	UndoBox move_history[MAX_GAME_MOVES]; // Fixed indices, easier to manage than vector

	int killer_moves[2][MAX_DEPTH]; // killer moves [id][ply]
	int history_moves[13][64]; // history moves [piece][square]
};

#endif // BOARD_HPP