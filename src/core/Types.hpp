#pragma once
#include <string>

struct Point {
    int r, c;
    bool operator==(const Point& o) const { return r == o.r && c == o.c; }
};

struct State {
    int r, c;
    int mask;
    bool operator==(const State& o) const {
        return r == o.r && c == o.c && mask == o.mask;
    }
};

struct Node {
    State state;
    int g;
    double h;
    std::string path;
};

struct IterationLog {
    int iteration;
    State state;
    int g;
    double h;
    std::string path;
};

struct SlideResult {
    State final_state;
    int cost_added;
    bool valid;
};
