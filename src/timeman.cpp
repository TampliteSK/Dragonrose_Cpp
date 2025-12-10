// timeman.cpp

#include "timeman.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>

// Get time in milliseconds
uint64_t get_time_ms() {
    auto now = std::chrono::steady_clock::now();  // Get the current time point
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch());  // Convert to milliseconds since epoch
    return static_cast<uint64_t>(duration.count());
}

// Allocate time based on the current ply, which is used to determine the phase of the game
// Returns the time allocated in milliseconds
int allocate_time(const Board& pos, int time) {
    // Time trouble case
    if (time < 15000) {  // 15s
        return time / 35;
    }

    // Based on https://www.chessprogramming.org/File:GrandmasterThinkingTime.jpg
    // We obtain 11.5%, 72.4%, 16.1% for opening, middlegame, endgame respectively based on our
    // thresholds Of course, the exact numbers are quite arbitrary, but the main idea lies in the
    // relative proportions
    constexpr uint8_t OPENING_PLY = 30;  // 15 moves
    constexpr uint8_t MIDGAME_PLY = 80;  // 40 moves
    int time_allocated = 0, phase_moves = 0;

    if (pos.his_ply <= OPENING_PLY) {
        // Opening phase (12% / 72% / 16%)
        phase_moves = (OPENING_PLY + 10 - pos.his_ply) / 2;  // Add an additional 10 ply as buffer
        time_allocated = time * 12 / (100 * phase_moves);
    } else if (pos.his_ply <= MIDGAME_PLY) {
        // Middlegame phase (72% / 16% -> 82% / 12%)
        phase_moves = (MIDGAME_PLY + 10 - pos.his_ply) / 2;
        time_allocated = time * 82 / (100 * phase_moves);
    } else {
        // Endgame phase (divide evenly rather conservatively)
        time_allocated = time / 35;
    }

    // std::cout << "Original time: " << time << ", Ply: " << (int)pos.his_ply << " | Allocated
    // time: " << time_allocated << "ms\n";
    return time_allocated;
}
