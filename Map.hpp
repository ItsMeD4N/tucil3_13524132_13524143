#pragma once
#include <string>
#include <vector>
#include "Types.hpp"

class Map {
public:
    int rows, cols;
    std::vector<std::string> grid;
    std::vector<std::vector<int>> costs;
    Point start;
    Point goal;
    int total_waypoints;
    std::vector<Point> waypoints;

    bool load(const std::string& filename);
    void printMap(const State& state) const;
};
