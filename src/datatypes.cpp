// datatypes.cpp

#include "datatypes.hpp"

//                   EMPTY,    wP,    wN,    wB,    wR,    wQ,    wK,    bP,    bN,    bB,    bR,    bQ,    bK
int piece_type[13] = { NONE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,  PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
int piece_col[13] = { BOTH, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK };
