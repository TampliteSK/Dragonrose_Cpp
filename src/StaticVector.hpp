// StaticVector.hpp
// A class for static vectors based on Stoat's impl (Ciekce's shogi engine)
// Used for move lists over std::vector for performance

#ifndef STATICVECTOR_HPP
#define STATICVECTOR_HPP

#include <array>
#include <cassert>

// Contrary to popular belief, there are positions with >256 legal moves
// Example: QQQQQQBk/Q6B/Q6Q/Q6Q/Q6Q/Q6Q/Q6Q/KQQQQQQQ w - - 0 1 (265 moves)
// Credits to Caissa for discovering the position (and by virtue DarkNeutrino for digging Caissa commits)
constexpr uint16_t MAX_LEGAL_MOVES = 280;

// Capacity: Maximum number of elements the static vector can hold
// m_size: Current number of elements in the static vector
template<typename T, size_t capacity>
class StaticVector {
    public:
        // General operations
        void push(const T &elem) {
            assert(m_size < capacity); // Debug safety
            m_data[m_size++] = elem;
        }

        void clear() { m_size = 0; }

        size_t size() const { return m_size; }

        // Iterators
        auto begin()       { return m_data.begin(); }
        auto end()         { return m_data.begin() + m_size; }
        auto begin() const { return m_data.begin(); }
        auto end()   const { return m_data.begin() + m_size; }

        // Direct access
        T& operator[](size_t i)       { assert(i < m_size); return m_data[i]; }
        const T& operator[](size_t i) const { assert(i < m_size); return m_data[i]; }

    private:
        std::array<T, capacity> m_data{};
        size_t m_size{0};
};

#endif // STATICVECTOR_HPP