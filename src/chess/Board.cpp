// Board.cpp

#include "Board.hpp"

#include <cstdlib>  // atoi()
#include <iostream>

#include "bitboard.hpp"
#include "movegen.hpp"
#include "moveio.hpp"
#include "zobrist.hpp"

/*
        Macro board manipulation
*/

void reset_board(Board *pos) {
    for (int sq = 0; sq < 64; ++sq) {
        pos->pieces[sq] = EMPTY;
    }

    for (int i = 0; i < 13; ++i) {
        pos->bitboards[i] = 0ULL;
        pos->piece_num[i] = 0;
        if (i < 3) {
            pos->occupancies[i] = 0ULL;
            pos->king_sq[i] = NO_SQ;
        }
    }

    pos->side = WHITE;
    pos->enpas = NO_SQ;
    pos->castle_perms = 0;
    pos->fifty_move = 0;
    pos->ply = 0;
    pos->his_ply = 0;
    pos->hash_key = 0ULL;

    for (int index = 0; index < 2; ++index) {
        for (int ply = 0; ply < MAX_DEPTH; ++ply) {
            pos->killer_moves[index][ply] = 0;
        }
    }
    for (int pce = 0; pce < 13; ++pce) {
        for (int sq = 0; sq < 64; ++sq) {
            pos->history_moves[pce][sq] = 0;
        }
    }

    pos->PV_array.length = 0;
    pos->PV_array.score = -INF_BOUND;
    for (int i = 0; i < MAX_DEPTH; ++i) {
        pos->PV_array.moves[i] = 0;  // NO_MOVE
    }

    for (UndoBox &box : pos->move_history) {
        box.castle_perms = 0;
        box.enpas = 0;
        box.fifty_move = 0;
        box.hash_key = 0ULL;
        box.move = 0;  // NO_MOVE
    }
}

// Update other variables of the board, for when only bitboards and occupancies were setup
void update_vars(Board *pos) {
    for (int pce = wP; pce <= bK; ++pce) {
        pos->piece_num[pce] = count_bits(pos->bitboards[pce]);
        if (piece_type[pce] == KING) {
            Bitboard copy = pos->bitboards[pce];
            pos->king_sq[piece_col[pce]] = pop_ls1b(copy);
        }
    }
}

// Rewritten from VICE parse_fen() function by Richard Allbert
void parse_fen(Board *pos, const std::string FEN) {
    if (FEN.length() <= 0) {
        std::cerr << "Board parse_fen() error: Invalid FEN length.\n";
    }

    reset_board(pos);
    int pfen = 0;  // Pointer to FEN character.

    /********************
    **  Parsing Pieces **
    ****************** */

    int rank = RANK_8;
    int file = FILE_A;
    int piece = 0;
    int count = 0;  // no. of consecutive empty squares / placeholder
    int sq = 0;

    while ((rank <= RANK_1) && pfen < (int)FEN.length()) {
        count = 1;

        switch (FEN[pfen]) {
            case 'p':
                piece = bP;
                break;
            case 'r':
                piece = bR;
                break;
            case 'n':
                piece = bN;
                break;
            case 'b':
                piece = bB;
                break;
            case 'k':
                piece = bK;
                break;
            case 'q':
                piece = bQ;
                break;
            case 'P':
                piece = wP;
                break;
            case 'R':
                piece = wR;
                break;
            case 'N':
                piece = wN;
                break;
            case 'B':
                piece = wB;
                break;
            case 'K':
                piece = wK;
                break;
            case 'Q':
                piece = wQ;
                break;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
                piece = EMPTY;
                count = FEN[pfen] - '0';  // number of consecutive empty squares
                break;

            case '/':
            case ' ':
                rank++;
                file = FILE_A;
                pfen++;
                continue;

            default:
                std::cerr << "Board parse_fen() error: Invalid FEN character: " << FEN[pfen]
                          << "\n";
                return;
        }

        // Putting pieces on the board
        for (int i = 0; i < count; i++) {
            sq = FR2SQ(file, rank);
            // Skips a file if empty square
            if (piece != EMPTY) {
                pos->pieces[sq] = piece;
                SET_BIT(pos->bitboards[piece], sq);
                SET_BIT(pos->occupancies[BOTH], sq);
                SET_BIT(pos->occupancies[piece_col[piece]], sq);
            }
            file++;
        }
        pfen++;
    }

    /********************
     * Parsing Misc Data *
     ****************** */

    // Side-to-move parsing
    pos->side = (FEN[pfen] == 'w') ? WHITE : BLACK;
    pfen += 2;

    // Castling perm parsing
    for (int i = 0; i < 4; i++) {
        if (FEN[pfen] == ' ') {
            break;
        }
        switch (FEN[pfen]) {
            case 'K':
                pos->castle_perms |= WKCA;
                break;
            case 'Q':
                pos->castle_perms |= WQCA;
                break;
            case 'k':
                pos->castle_perms |= BKCA;
                break;
            case 'q':
                pos->castle_perms |= BQCA;
                break;
            default:
                break;
        }
        pfen++;
    }
    pfen++;

    // En passant parsing
    if (FEN[pfen] != '-') {
        file = FEN[pfen] - 'a';
        rank = 7 - int(FEN[pfen + 1] - '1');
        pos->enpas = FR2SQ(file, rank);
        pfen += 3;
    } else {
        pfen += 2;
    }

    // Fifty-move counter parsing
    uint16_t half_moves = 0;
    while (pfen < (int)FEN.length() && FEN[pfen] != ' ') {
        half_moves = half_moves * 10 + (FEN[pfen] - '0');
        pfen++;
    }
    pos->fifty_move = half_moves;
    pfen++;  // Move past the space

    // Full move number parsing
    uint16_t full_move = 0;
    while (pfen < (int)FEN.length()) {
        if (FEN[pfen] == ' ') {
            break;
        }
        full_move = full_move * 10 + (FEN[pfen] - '0');
        pfen++;
    }
    pos->ply = (full_move - 1) * 2 +
               (pos->side == BLACK ? 1 : 0);  // FEN fullmove starts at 1 → ply starts at 0
    pos->his_ply = pos->ply;

    pos->hash_key = generate_hash_key(pos);  // Get Zobrist key for the position

    update_vars(pos);
}

/*
        Output functions (debug)
*/

void print_board(const Board *pos) {
    std::cout << "\n";

    for (int rank = RANK_8; rank <= RANK_1; rank++) {
        for (int file = FILE_A; file <= FILE_H; file++) {
            if (file == 0) std::cout << "  " << (8 - rank) << " ";  // Print rank number

            uint8_t sq = FR2SQ(file, rank);
            int piece = pos->pieces[sq];
            std::cout << " " << ascii_pieces[piece];
        }

        std::cout << "\n";
    }

    std::cout << "\n     a b c d e f g h\n\n";
    std::cout << "           Side: " << (pos->side == 0 ? "White" : "Black") << "\n";
    std::cout << "     En passant: " << (pos->enpas != NO_SQ ? ascii_squares[pos->enpas] : "N/A")
              << "\n";
    std::cout << "50-move counter: " << (int)pos->fifty_move << "\n";
    std::cout << "            Ply: " << (int)pos->ply << "\n";
    std::cout << "    History ply: " << (int)pos->his_ply << "\n";

    // Print castling rights
    std::cout << "       Castling: " << ((pos->castle_perms & WKCA) ? 'K' : '-')
              << ((pos->castle_perms & WQCA) ? 'Q' : '-')
              << ((pos->castle_perms & BKCA) ? 'k' : '-')
              << ((pos->castle_perms & BQCA) ? 'q' : '-') << "\n";

    std::cout << "       Hash key: " << std::hex << pos->hash_key << "\n\n";
    std::cout << std::dec;  // Reset output to base 10
}

void print_move_history(const Board *pos) {
    if (pos->his_ply == 0) {
        std::cout << "print_move_history() warning: No move history available.\n";
        return;
    }

    std::cout << "Move history: ";
    for (int i = 0; i < pos->his_ply; ++i) {
        std::cout << print_move(pos->move_history[i].move) << " ";
    }
    std::cout << "\n";
}

/*
        Misc function
*/

// Check if every attribute is the same. Returns true if they are
bool check_boards(const Board *pos1, const Board *pos2) {
    // Compare pieces
    for (int i = 0; i < 64; ++i) {
        if (pos1->pieces[i] != pos2->pieces[i]) {
            std::cout << "Discrepancy of piece at square " << i
                      << ": Left board: " << ascii_pieces[pos1->pieces[i]]
                      << ". Right board: " << ascii_pieces[pos1->pieces[i]] << "\n";
            return false;
        }
    }

    // Compare bitboards
    for (int i = 0; i < 13; ++i) {
        if (pos1->bitboards[i] != pos2->bitboards[i]) {
            std::cout << "Discrepancy of bitboard at index " << i << "\n";
            return false;
        }
    }

    // Compare occupancies
    for (int i = 0; i < 3; ++i) {
        if (pos1->occupancies[i] != pos2->occupancies[i]) {
            std::cout << "Discrepancy of occupancies at index " << i << "\n";
            return false;
        }
    }

    // Compare king squares
    for (int i = 0; i < 3; ++i) {
        if (pos1->king_sq[i] != pos2->king_sq[i]) {
            std::cout << "Discrepancy of king_sq at index " << i
                      << ": Left board: " << ascii_squares[pos1->king_sq[i]]
                      << ". Right board: " << ascii_squares[pos2->king_sq[i]] << "\n";
            return false;
        }
    }

    // Compare other attributes
    if (pos1->side != pos2->side || pos1->enpas != pos2->enpas ||
        pos1->castle_perms != pos2->castle_perms || pos1->fifty_move != pos2->fifty_move ||
        pos1->ply != pos2->ply || pos1->his_ply != pos2->his_ply ||
        pos1->hash_key != pos2->hash_key) {
        std::cout
            << "Either side, enpas, castle_perms, fiftymove, ply, hisply or hashkey is different\n";
        return false;
    }

    return true;
}