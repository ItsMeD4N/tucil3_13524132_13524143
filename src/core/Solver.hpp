#pragma once
#include <string>
#include <vector>
#include "Types.hpp"
#include "Map.hpp"

SlideResult performSlide(const Map& map, State current, char dir);
Node solve(
    const Map& map,
    const std::string& algo,
    const std::string& heuristic_str,
    int& iterations,
    std::vector<IterationLog>* iteration_logs = nullptr
);