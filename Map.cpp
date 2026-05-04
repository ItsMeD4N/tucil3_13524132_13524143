#include "Map.hpp"
#include <iostream>
#include <fstream>

using namespace std;

bool Map::load(const string& filename) {
    ifstream file(filename);
    if (!file) return false;

    if (!(file >> rows >> cols)) return false;

    grid.resize(rows);
    for (int r = 0; r < rows; ++r) {
        file >> grid[r];
    }

    costs.resize(rows, vector<int>(cols));
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            file >> costs[r][c];
        }
    }

    vector<Point> temp_wp(10, Point{-1, -1});
    int max_wp = -1;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < grid[r].size(); ++c) {
            char ch = grid[r][c];
            if (ch == 'Z') {
                start = {r, c};
                grid[r][c] = '*';
            } else if (ch == 'O') {
                goal = {r, c};
            } else if (ch >= '0' && ch <= '9') {
                int val = ch - '0';
                temp_wp[val] = {r, c};
                if (val > max_wp) max_wp = val;
            }
        }
    }

    total_waypoints = max_wp + 1;
    waypoints.clear();
    for (int i = 0; i < total_waypoints; ++i) {
        waypoints.push_back(temp_wp[i]);
    }

    return true;
}

void Map::printMap(const State& state) const {
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < grid[r].size(); ++c) {
            if (r == state.r && c == state.c) {
                cout << 'Z';
            } else {
                char ch = grid[r][c];
                if (ch >= '0' && ch <= '9') {
                    if ((ch - '0') < state.mask) {
                        cout << '*';
                    } else {
                        cout << ch;
                    }
                } else {
                    cout << ch;
                }
            }
        }
        cout << '\n';
    }
}