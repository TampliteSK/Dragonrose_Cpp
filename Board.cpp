// Board.cpp

#include <iostream>
#include <cstdlib> // atoi()
#include "Board.hpp"
#include "bitboard.hpp"
#include "zobrist.hpp"
#include "moveio.hpp"

// Constructors
Board::Board() {
	reset_board();
}

Board::Board(const std::string FEN) {
	reset_board();
	parse_fen(FEN);
}

// Copy constructor
Board::Board(const Board& other) {
	std::copy(std::begin(other.pieces), std::end(other.pieces), pieces);
	std::copy(std::begin(other.bitboards), std::end(other.bitboards), bitboards);
	std::copy(std::begin(other.occupancies), std::end(other.occupancies), occupancies);

	std::copy(std::begin(other.piece_num), std::end(other.piece_num), piece_num);

	std::copy(std::begin(other.king_sq), std::end(other.king_sq), king_sq);
	side = other.side;
	enpas = other.enpas;
	castle_perms = other.castle_perms;
	fifty_move = other.fifty_move;

	ply = other.ply;
	his_ply = other.his_ply;
	hash_key = other.hash_key;
	std::copy(std::begin(other.move_history), std::end(other.move_history), move_history);

	std::copy(&other.killer_moves[0][0], &other.killer_moves[0][0] + 2 * MAX_DEPTH, &killer_moves[0][0]);
	std::copy(&other.history_moves[0][0], &other.history_moves[0][0] + 13 * 64, &history_moves[0][0]);
}

/*
	Board manipulation
*/

// Clone method
Board* Board::clone() const {
	return new Board(*this); // Calls the copy constructor
}

void Board::reset_board() {

	for (int sq = 0; sq < 64; ++sq) {
		pieces[sq] = EMPTY;
	}

	for (int i = 0; i < 13; ++i) {
		bitboards[i] = 0ULL;
        piece_num[i] = 0;
		if (i < 3) {
			occupancies[i] = 0ULL;
            king_sq[i] = NO_SQ;
		}
	}

	side = WHITE;
	enpas = NO_SQ;
	castle_perms = 0;
	fifty_move = 0;

	ply = 0;
	his_ply = 0;
	hash_key = 0ULL;

	for (int index = 0; index < 2; ++index) {
		for (int ply = 0; ply < MAX_DEPTH; ++ply) {
			killer_moves[index][ply] = 0;
		}
	}

	for (int pce = 0; pce < 13; ++pce) {
		for (int sq = 0; sq < 64; ++sq) {
			history_moves[pce][sq] = 0;
		}
	}

	for (UndoBox box : move_history) {
		box.castle_perms = 0;
		box.enpas = 0;
		box.fifty_move = 0;
		box.hash_key = 0ULL;
		box.move = 0; // NO_MOVE
	}
}

// Update other variables of the board, for when only bitboards and occupancies were setup
void Board::update_vars() {
	for (int pce = wP; pce <= bK; ++pce) {
		piece_num[pce] = count_bits(bitboards[pce]);
		if (piece_type[pce] == KING) {
			Bitboard copy = bitboards[pce];
			king_sq[piece_col[pce]] = pop_ls1b(copy);
		}
	}
}

// Rewritten from VICE parse_fen() function by Richard Allbert
void Board::parse_fen(const std::string FEN) {

	if (FEN.length() <= 0) {
		std::cerr << "Board parse_fen() error: Invalid FEN length.\n";
	}

	reset_board();
	int pfen = 0; // Pointer to FEN character.

	/********************
	**  Parsing Pieces **
	****************** */

	int rank  = RANK_8;
	int file  = FILE_A;
	int piece = 0;
	int count = 0; // no. of consecutive empty squares / placeholder
	int sq    = 0;

	while ((rank <= RANK_1) && pfen < FEN.length()) {
		count = 1;

		switch (FEN[pfen]) {
			case 'p': piece = bP; break;
			case 'r': piece = bR; break;
			case 'n': piece = bN; break;
			case 'b': piece = bB; break;
			case 'k': piece = bK; break;
			case 'q': piece = bQ; break;
			case 'P': piece = wP; break;
			case 'R': piece = wR; break;
			case 'N': piece = wN; break;
			case 'B': piece = wB; break;
			case 'K': piece = wK; break;
			case 'Q': piece = wQ; break;

			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
				piece = EMPTY;
				count = FEN[pfen] - '0'; // number of consecutive empty squares
				break;

			case '/':
			case ' ':
				rank++;
				file = FILE_A;
				pfen++;
				continue;

			default:
				std::cerr << "Board parse_fen() error: Invalid FEN character: " << FEN[pfen] << "\n";
				return;
		}

		// Putting pieces on the board
		for (int i = 0; i < count; i++) {
			sq = FR2SQ(file, rank);
			// Skips a file if empty square
			if (piece != EMPTY) { 
				pieces[sq] = piece;
				SET_BIT(bitboards[piece], sq);
				SET_BIT(occupancies[BOTH], sq);
				SET_BIT(occupancies[piece_col[piece]], sq);
			}
			file++;
		}
		pfen++;
	}

	/********************
	* Parsing Misc Data *
	****************** */

	// Side-to-move parsing
	side = (FEN[pfen] == 'w') ? WHITE : BLACK;
	pfen += 2;

	// Castling perm parsing
	for (int i = 0; i < 4; i++) {
		if (FEN[pfen] == ' ') {
			break;
		}
		switch (FEN[pfen]) {
			case 'K': castle_perms |= WKCA; break;
			case 'Q': castle_perms |= WQCA; break;
			case 'k': castle_perms |= BKCA; break;
			case 'q': castle_perms |= BQCA; break;
			default: break;
		}
		pfen++;
	}
	pfen++;

	// En passant parsing
	if (FEN[pfen] != '-') {
		file = FEN[pfen]     - 'a';
		rank = 8 - int(FEN[pfen + 1] - '1');
		enpas = FR2SQ(file, rank);
		pfen += 3;
	}
	else {
		pfen += 2;
	}

	// Fifty-move counter parsing
	uint16_t half_moves = 0;
	while (pfen < FEN.length() && FEN[pfen] != ' ') {
		half_moves = half_moves * 10 + (FEN[pfen] - '0');
		pfen++;
	}
	fifty_move = half_moves;
	pfen++; // Move past the space

	// Full move number parsing
	uint16_t full_move = 0;
	while (pfen < FEN.length()) {
		if (FEN[pfen] == ' ') {
			break;
		}
		full_move = full_move * 10 + (FEN[pfen] - '0');
		pfen++;
	}
	// ply = his_ply = full_move;


	// Zobrist key
	hash_key = generate_hash_key(*this);
	
	update_vars();
}

void Board::print_board() const {
    std::cout << "\n";

    for (int rank = RANK_8; rank <= RANK_1; rank++) {
        for (int file = FILE_A; file <= FILE_H; file++) {
            if (file == 0)
                std::cout << "  " << (8 - rank) << " "; // Print rank number

			uint8_t sq = FR2SQ(file, rank);
            int piece = pieces[sq];
            std::cout << " " << ascii_pieces[piece];
        }

        std::cout << "\n";
    }

    std::cout << "\n     a b c d e f g h\n\n";
    std::cout << "           Side: " << (side == 0 ? "White" : "Black") << "\n";
    std::cout << "     En passant: " << (enpas != NO_SQ ? ascii_squares[enpas] : "N/A") << "\n";
	std::cout << "50-move counter: " << (int)fifty_move << "\n";
	std::cout << "            Ply: " << (int)ply << "\n";
	std::cout << "    History ply: " << (int)his_ply << "\n";

    // Print castling rights
    std::cout << "       Castling: "
        << ((castle_perms & WKCA) ? 'K' : '-')
        << ((castle_perms & WQCA) ? 'Q' : '-')
        << ((castle_perms & BKCA) ? 'k' : '-')
        << ((castle_perms & BQCA) ? 'q' : '-')
        << "\n";

    std::cout << "       Hash key: " << std::hex << hash_key << "\n\n";
	std::cout << std::dec; // Reset output to base 10
}

void Board::print_move_history() const {
	std::cout << "Move history: ";
	for (int i = 0; i < his_ply; ++i) {
		std::cout << print_move(move_history[i].move) << " ";
	}
	std::cout << "\n";
}

/*
	Getter functions
*/

uint8_t Board::get_piece(uint8_t sq) const {
	return pieces[sq];
}
Bitboard Board::get_bitboard(uint8_t pce) const {
	return bitboards[pce];
}
Bitboard Board::get_occupancy(uint8_t col) const {
	return occupancies[col];
}

uint8_t Board::get_piece_num(uint8_t pce) const {
	return piece_num[pce];
}

uint8_t Board::get_king_sq(uint8_t col) const {
	return king_sq[col];
}
uint8_t Board::get_side() const {
	return side;
}
uint8_t Board::get_enpas() const {
	return enpas;
}
uint8_t Board::get_castle_perms() const {
	return castle_perms;
}
uint8_t Board::get_fifty_move() const {
	return fifty_move;
}

uint8_t Board::get_ply() const {
	return ply;
}
uint16_t Board::get_his_ply() const {
	return his_ply;
}
uint64_t Board::get_hash_key() const {
	return hash_key;
}
UndoBox* Board::get_move_history() const {
	UndoBox* history = new UndoBox[MAX_GAME_MOVES];
	for (int i = 0; i < MAX_GAME_MOVES; ++i) {
		history[i] = move_history[i];
	}
	return history;
}

int Board::get_killer_move(uint8_t id, uint8_t depth) const {
	return killer_moves[id][depth];
}
int Board::get_history_move(uint8_t pce, uint8_t sq) const {
	return history_moves[pce][sq];
}

/*
	Setters
*/

void Board::set_piece(uint8_t sq, uint8_t new_pce) {
	if (sq < 64) {
		pieces[sq] = new_pce;
	}
}
void Board::set_bitboard(uint8_t pce, uint8_t index, uint8_t new_bit) {
	if (pce < 13) {
		if (new_bit) {
			bitboards[pce] |= (1ULL << index); // Set the bit
		}
		else {
			bitboards[pce] &= ~(1ULL << index); // Clear the bit
		}
	}
}
void Board::set_occupancy(uint8_t col, uint8_t index, uint8_t new_bit) {
	if (col < 3) {
		if (new_bit) {
			occupancies[col] |= (1ULL << index); // Set the bit
		}
		else {
			occupancies[col] &= ~(1ULL << index); // Clear the bit
		}
	}
}

void Board::set_piece_num(uint8_t pce, uint8_t new_num) {
	if (pce < 13) {
		piece_num[pce] = new_num;
	}
}

void Board::set_king_sq(uint8_t col, uint8_t new_sq) {
	if (col < 3) {
		king_sq[col] = new_sq;
	}
}
void Board::set_side(uint8_t new_side) {
	side = new_side;
}
void Board::set_enpas(uint8_t new_sq) {
	enpas = new_sq;
}
void Board::set_castle_perms(uint8_t new_perms) {
	castle_perms = new_perms;
}
void Board::set_fifty_move(uint8_t new_count) {
	fifty_move = new_count;
}

void Board::set_ply(uint8_t new_ply) {
	ply = new_ply;
}
void Board::set_his_ply(uint16_t new_his_ply) {
	his_ply = new_his_ply;
}
void Board::set_hash_key(uint64_t new_key) {
	hash_key = new_key;
}
void Board::set_move_history(int index, UndoBox new_box) {
	move_history[index] = new_box;
}

void Board::set_killer_move(uint8_t id, uint8_t depth, int new_move) {
	if (id < 2 && depth < MAX_DEPTH) {
		killer_moves[id][depth] = new_move;
	}
}
void Board::set_history_move(uint8_t pce, uint8_t sq, int new_move) {
	if (pce < 13 && sq < 64) {
		history_moves[pce][sq] = new_move;
	}
}

/*
	Equality Overload
*/

// Check if every attribute is the same
bool Board::operator==(const Board& other) const {

	// Compare pieces
	for (int i = 0; i < 64; ++i) {
		if (this->pieces[i] != other.pieces[i]) {
			return false;
		}
	}

	// Compare bitboards
	for (int i = 0; i < 13; ++i) {
		if (this->bitboards[i] != other.bitboards[i]) {
			return false;
		}
	}

	// Compare occupancies
	for (int i = 0; i < 3; ++i) {
		if (this->occupancies[i] != other.occupancies[i]) {
			return false;
		}
	}

	// Compare piece numbers
	for (int i = 0; i < 13; ++i) {
		if (this->piece_num[i] != other.piece_num[i]) {
			return false;
		}
	}

	// Compare king squares
	for (int i = 0; i < 3; ++i) {
		if (this->king_sq[i] != other.king_sq[i]) {
			return false;
		}
	}

	// Compare other attributes
	if (this->side != other.side ||
		this->enpas != other.enpas ||
		this->castle_perms != other.castle_perms ||
		this->fifty_move != other.fifty_move ||
		this->ply != other.ply ||
		this->his_ply != other.his_ply ||
		this->hash_key != other.hash_key) {
		return false;
	}

	// Compare move history
	for (int i = 0; i < MAX_GAME_MOVES; ++i) {
		if (this->move_history[i].move		   != other.move_history[i].move ||
			this->move_history[i].castle_perms != other.move_history[i].castle_perms ||
			this->move_history[i].enpas        != other.move_history[i].enpas ||
			this->move_history[i].fifty_move   != other.move_history[i].fifty_move ||
			this->move_history[i].hash_key     != other.move_history[i].hash_key) {
			return false;
		}
	}

	return true;
}
