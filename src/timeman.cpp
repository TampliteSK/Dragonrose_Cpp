// timeman.cpp

#include <cstdint>
#include <chrono>
#include "timeman.hpp"

// Get time in milliseconds
uint64_t get_time_ms() {
    auto now = std::chrono::steady_clock::now(); // Get the current time point
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()); // Convert to milliseconds since epoch
    return static_cast<uint64_t>(duration.count());
}

int allocate_time(const Board* pos, int time) {

    int time_allocated = 0;

    // Time trouble
    if (time < 30000) { // 30s
        return time / 60;
    }
    else {
        if (pos->his_ply <= 30) {
            // Opening phase (10% / 45% / 45%)
            int phase_moves = (40 - pos->his_ply) / 2; // Add an additional 6 moves as buffer
            time_allocated = time * 10 / (100 * phase_moves);
        }
        else if (pos->his_ply <= 70) {
            // Middlegame phase (45% / 45% -> 50% / 50%)
            int phase_moves = (80 - pos->his_ply) / 2;
            time_allocated = time * 50 / (100 * phase_moves);
        }
        else {
            // Endgame phase (divide evenly rather conservatively)
            time_allocated = time / 35;
        }
    }

    return time_allocated + 50; // 50ms buffer
}