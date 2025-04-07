// movegen.hpp

#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <vector>
#include "Board.hpp"

typedef struct {
    int move;
    int score;
} Move;

// Functions
void generate_moves(const Board *pos, std::vector<Move>& move_list, bool noisy_only);
void sort_moves(const Board *pos, std::vector<Move>& move_list);

/*
          binary move bits                               hexidecimal constants

    0000 0000 0000 0000 0000 0011 1111    source square       0x3f
    0000 0000 0000 0000 1111 1100 0000    target square       0xfc0
    0000 0000 0000 1111 0000 0000 0000    piece               0xf000
    0000 0000 1111 0000 0000 0000 0000    promoted piece      0xf0000
    0000 1111 0000 0000 0000 0000 0000    captured piece      0xf00000
    0001 0000 0000 0000 0000 0000 0000    double push flag    0x1000000
    0010 0000 0000 0000 0000 0000 0000    enpassant flag      0x2000000
    0100 0000 0000 0000 0000 0000 0000    castling flag       0x4000000
*/

// encode move
#define encode_move(source, target, piece, promoted, capture, double_adv, enpassant, castling) \
    (source) |           \
    ((target) << 6) |      \
    ((piece) << 12) |      \
    ((promoted) << 16) |   \
    ((capture) << 20) |    \
    ((double_adv) << 24) | \
    ((enpassant) << 25) |  \
    ((castling) << 26)     \

// extract source square, target square, piece, promoted piece, capture flag, double pawn push flag, enpassant flag, and castling flag
#define get_move_source(move) (move & 0x3f)
#define get_move_target(move) ((move & 0xfc0) >> 6)
#define get_move_piece(move) ((move & 0xf000) >> 12)
#define get_move_promoted(move) ((move & 0xf0000) >> 16)
#define get_move_captured(move) ((move & 0xf00000) >> 20)
#define get_move_double(move) (move & 0x1000000)
#define get_move_enpassant(move) (move & 0x2000000)
#define get_move_castling(move) (move & 0x4000000)

/*
        Most Valuable Victim & Least Valuable Attacker

    (Victims) Pawn Knight Bishop   Rook  Queen   King
  (Attackers)
        Pawn   105    205    305    405    505    605
      Knight   104    204    304    404    504    604
      Bishop   103    203    303    403    503    603
        Rook   102    202    302    402    502    602
       Queen   101    201    301    401    501    601
        King   100    200    300    400    500    600

*/

// MVV LVA [attacker][victim]
static int mvv_lva[12][12] = {
    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};


#endif // MOVEGEN_HPP