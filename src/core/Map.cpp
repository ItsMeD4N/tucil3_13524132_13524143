#include "Map.hpp"
#include <iostream>
#include <fstream>
#include <cctype>

using namespace std;

bool Map::load(const string& filename) {
    last_error.clear();
    ifstream file(filename);
    if (!file) {
        last_error = "File tidak dapat dibuka.";
        return false;
    }

    if (!(file >> rows >> cols) || rows <= 0 || cols <= 0) {
        last_error = "Ukuran board tidak valid.";
        return false;
    }

    grid.resize(rows);
    for (int r = 0; r < rows; ++r) {
        if (!(file >> grid[r]) || static_cast<int>(grid[r].size()) != cols) {
            last_error = "Baris board tidak sesuai ukuran kolom.";
            return false;
        }
    }

    costs.resize(rows, vector<int>(cols));
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (!(file >> costs[r][c]) || costs[r][c] < 0) {
                last_error = "Data cost tidak lengkap atau bernilai negatif.";
                return false;
            }
        }
    }

    vector<Point> temp_wp(10, Point{-1, -1});
    int max_wp = -1;
    int start_count = 0;
    int goal_count = 0;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            char ch = grid[r][c];
            bool is_digit = (ch >= '0' && ch <= '9');
            bool is_allowed = (ch == '*' || ch == 'X' || ch == 'L' || ch == 'O' || ch == 'Z' || is_digit);
            if (!is_allowed) {
                last_error = "Terdapat karakter tile tidak valid.";
                return false;
            }

            if (ch == 'Z') {
                start_count++;
                start = {r, c};
                grid[r][c] = '*';
            } else if (ch == 'O') {
                goal_count++;
                goal = {r, c};
            } else if (is_digit) {
                int val = ch - '0';
                if (temp_wp[val].r != -1) {
                    last_error = "Waypoint angka duplikat.";
                    return false;
                }
                temp_wp[val] = {r, c};
                if (val > max_wp) max_wp = val;
            }
        }
    }

    if (start_count != 1 || goal_count != 1) {
        last_error = "Board harus memiliki tepat satu titik start (Z) dan satu goal (O).";
        return false;
    }

    total_waypoints = max_wp + 1;
    waypoints.clear();
    for (int i = 0; i < total_waypoints; ++i) {
        if (temp_wp[i].r == -1) {
            last_error = "Waypoint angka harus berurutan dari 0 tanpa ada yang hilang.";
            return false;
        }
        waypoints.push_back(temp_wp[i]);
    }

    return true;
}

void Map::printMap(const State& state) const {
    printMap(cout, state);
}

void Map::printMap(std::ostream& out, const State& state) const {
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (r == state.r && c == state.c) {
                out << 'Z';
            } else {
                char ch = grid[r][c];
                if (ch >= '0' && ch <= '9') {
                    if ((ch - '0') < state.mask) {
                        out << '*';
                    } else {
                        out << ch;
                    }
                } else {
                    out << ch;
                }
            }
        }
        out << '\n';
    }
}