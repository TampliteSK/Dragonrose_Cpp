// StaticVector.hpp

#ifndef STATICVECTOR_HPP
#define STATICVECTOR_HPP

#include <cstddef>
#include <cstdint>
#include <cstddef>

/*
    The highest number of legal moves for a reachable position is 218,
    but there are certain custom legal positions with more moves:
    Example 1 from Caissa: QQQQQQBk/Q6B/Q6Q/Q6Q/Q6Q/Q6Q/Q6Q/KQQQQQQQ w - - 0 1 (265 moves)
    Example 2 (similar position): QQQQQQBk/Q5RB/Q6Q/Q6Q/Q6Q/Q6Q/Q6Q/KQQQQQQQ w - - 0 1 (271 moves)
    With this in mind, Dragonrose will support up to 280 pseudo-legal moves in a position for good
   measure. See this Lichess blog post by Tobs40 for more info:
    https://lichess.org/@/Tobs40/blog/why-a-reachable-position-can-have-at-most-218-playable-moves/a5xdxeqs
*/
constexpr uint16_t MAX_PSEUDO_MOVES = 280;

typedef struct {
    int move;
    int score;
} Move;

template <typename T, std::size_t capacity>
struct StaticVector {
    // Initialise everything at creation
    T moves[capacity] = {};
    uint16_t length = 0;
};

typedef StaticVector<Move, MAX_PSEUDO_MOVES> MoveList;

#endif  // STATICVECTOR_HPP