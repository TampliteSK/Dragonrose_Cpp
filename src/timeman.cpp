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