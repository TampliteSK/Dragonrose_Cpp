// StaticVector.hpp

#ifndef STATICVECTOR_HPP
#define STATICVECTOR_HPP

// Contrary to popular belief, there are positions with >256 legal moves
// Example: QQQQQQBk/Q6B/Q6Q/Q6Q/Q6Q/Q6Q/Q6Q/KQQQQQQQ w - - 0 1 (265 moves)
// Credits to Caissa for discovering the position (and by virtue DarkNeutrino for digging Caissa commits)
constexpr uint16_t MAX_LEGAL_MOVES = 280;

typedef struct {
    int move;
    int score;
} Move;

template<typename T, size_t capacity>
struct StaticVector {  // Named struct
    T moves[capacity];
    uint16_t length = 0;
};

typedef StaticVector<Move, MAX_LEGAL_MOVES> MoveList;

#endif // STATICVECTOR_HPP