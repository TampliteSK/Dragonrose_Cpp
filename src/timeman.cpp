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

    // TIME TROUBLE CAES: Allocate time dynamically based on time left
    const int TIME_TROUBLE_THRESHOLD = 15000; // in ms
    if (time < TIME_TROUBLE_THRESHOLD) {
        // Linear interpolation formula:
        // start_val + (end_val - start_val) * (1.0 - (current_time / max_time))
        const double START = 10.0;
        const double END = 20.0;
        double dynamic_divisor = START + (END - START) * (1.0 - (double)std::max(0, time) / TIME_TROUBLE_THRESHOLD);
        return time / (int)dynamic_divisor;
    }


    // OPENING / MIDDLEGAME PHASE: Allocate time dynamically based on game phase
    // (Based on https://www.chessprogramming.org/File:GrandmasterThinkingTime.jpg)
    // We obtain 11.5%, 72.4%, 16.1% for opening, middlegame, endgame respectively based on our
    // thresholds. Of course, the exact numbers are quite arbitrary, but the main idea lies in the
    // relative proportions
    const Phase stages[] = { {30, 12}, {80, 82} }; // Opening, Middlegame

    for (const auto& stage : stages) {
        if (pos.his_ply <= stage.limit) {
            int moves = (stage.limit + 10 - pos.his_ply) / 2;
            return (time * stage.weight) / (100 * std::max(1, moves));
        }
    }

    // ENDGAME PHASE: Flat divisor until time trouble
    return time / 35;
}
