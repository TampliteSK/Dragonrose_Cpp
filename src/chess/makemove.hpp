// makemove.hpp

#ifndef MAKEMOVE_HPP
#define MAKEMOVE_HPP

#include <cstdint>

#include "Board.hpp"

#define PIN_ILLEGAL_TEST "r1bqk1nr/pppp1ppp/2n5/4p3/1b2P3/2NP4/PPP2PPP/R1BQKBNR w KQkq - 3"

constexpr int NO_MOVE = 0;

// Functions
void take_move(Board *pos);
bool make_move(Board *pos, int move);
void make_null_move(Board *pos);
void take_null_move(Board *pos);

/*
                           castling   move     in      in
                              right update     binary  decimal

 king & rooks didn't move:     1111 & 1111  =  1111    15

        white king  moved:     1111 & 1100  =  1100    12
  white king's rook moved:     1111 & 1110  =  1110    14
 white queen's rook moved:     1111 & 1101  =  1101    13

         black king moved:     1111 & 0011  =  1011    3
  black king's rook moved:     1111 & 1011  =  1011    11
 black queen's rook moved:     1111 & 0111  =  0111    7

*/

// castling rights update constants
const int castling_rights[64] = {7,  15, 15, 15, 3,  15, 15, 11, 15, 15, 15, 15, 15, 15, 15, 15,
                                 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                                 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                                 15, 15, 15, 15, 15, 15, 15, 15, 13, 15, 15, 15, 12, 15, 15, 14};

#endif  // MAKEMOVE_HPP