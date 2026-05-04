#include "Solver.hpp"
#include <queue>
#include <cmath>

using namespace std;

struct CmpUCS {
    bool operator()(const Node& a, const Node& b) const {
        return a.g > b.g;
    }
};

struct CmpGBFS {
    bool operator()(const Node& a, const Node& b) const {
        return a.h > b.h;
    }
};

struct CmpAStar {
    bool operator()(const Node& a, const Node& b) const {
        if (a.g + a.h == b.g + b.h) return a.g > b.g;
        return (a.g + a.h) > (b.g + b.h);
    }
};

SlideResult performSlide(const Map& map, State current, char dir) {
    int dr = 0, dc = 0;
    if (dir == 'U') dr = -1;
    if (dir == 'D') dr = 1;
    if (dir == 'L') dc = -1;
    if (dir == 'R') dc = 1;

    int r = current.r;
    int c = current.c;
    int mask = current.mask;
    int cost = 0;
    bool moved = false;

    while (true) {
        int nr = r + dr;
        int nc = c + dc;

        if (nr < 0 || nr >= map.rows || nc < 0 || nc >= map.grid[nr].size()) {
            return {State{r, c, mask}, 0, false};
        }

        char cell = map.grid[nr][nc];

        if (cell == 'X') {
            break;
        }

        moved = true;
        cost += map.costs[nr][nc];
        r = nr;
        c = nc;

        if (cell == 'L') {
            return {State{r, c, mask}, 0, false};
        }

        if (cell >= '0' && cell <= '9') {
            int wp = cell - '0';
            if (wp > mask) {
                return {State{r, c, mask}, 0, false};
            } else if (wp == mask) {
                mask++;
            }
        }
    }

    if (!moved) return {State{r, c, mask}, 0, false};

    return {State{r, c, mask}, cost, true};
}

double computeHeuristic(int h_type, const Map& map, const State& state) {
    Point target;
    if (state.mask < map.total_waypoints) {
        target = map.waypoints[state.mask];
    } else {
        target = map.goal;
    }

    double dx = abs(state.r - target.r);
    double dy = abs(state.c - target.c);

    if (h_type == 1) {
        return dx + dy;
    } else if (h_type == 2) {
        return sqrt(dx * dx + dy * dy);
    } else if (h_type == 3) {
        int unvisited = map.total_waypoints - state.mask;
        return unvisited * 10.0 + (dx + dy);
    }
    return 0;
}

template <typename Cmp>
Node runSearch(const Map& map, int h_type, int& iterations, bool use_h, bool is_gbfs) {
    priority_queue<Node, vector<Node>, Cmp> pq;
    vector<vector<vector<int>>> best_cost(
        map.rows, vector<vector<int>>(map.cols, vector<int>(map.total_waypoints + 1, 1e9))
    );
    vector<vector<vector<bool>>> visited(
        map.rows, vector<vector<bool>>(map.cols, vector<bool>(map.total_waypoints + 1, false))
    );

    Node start_node;
    start_node.state = {map.start.r, map.start.c, 0};
    start_node.g = 0;
    start_node.h = use_h ? computeHeuristic(h_type, map, start_node.state) : 0;
    start_node.path = "";

    pq.push(start_node);
    if (!is_gbfs) best_cost[start_node.state.r][start_node.state.c][0] = 0;

    iterations = 0;
    char dirs[] = {'U', 'D', 'L', 'R'};

    while (!pq.empty()) {
        Node curr = pq.top();
        pq.pop();

        if (is_gbfs) {
            if (visited[curr.state.r][curr.state.c][curr.state.mask]) continue;
            visited[curr.state.r][curr.state.c][curr.state.mask] = true;
        } else {
            if (curr.g > best_cost[curr.state.r][curr.state.c][curr.state.mask]) continue;
        }

        iterations++;

        if (curr.state.mask == map.total_waypoints && map.grid[curr.state.r][curr.state.c] == 'O') {
            return curr;
        }

        for (char dir : dirs) {
            SlideResult res = performSlide(map, curr.state, dir);
            if (!res.valid) continue;

            int new_g = curr.g + res.cost_added;

            if (is_gbfs) {
                if (!visited[res.final_state.r][res.final_state.c][res.final_state.mask]) {
                    Node next_node;
                    next_node.state = res.final_state;
                    next_node.g = new_g;
                    next_node.h = computeHeuristic(h_type, map, res.final_state);
                    next_node.path = curr.path + dir;
                    pq.push(next_node);
                }
            } else {
                if (new_g < best_cost[res.final_state.r][res.final_state.c][res.final_state.mask]) {
                    best_cost[res.final_state.r][res.final_state.c][res.final_state.mask] = new_g;
                    Node next_node;
                    next_node.state = res.final_state;
                    next_node.g = new_g;
                    next_node.h = use_h ? computeHeuristic(h_type, map, res.final_state) : 0;
                    next_node.path = curr.path + dir;
                    pq.push(next_node);
                }
            }
        }
    }
    return Node{{-1, -1, -1}, -1, -1, ""};
}

Node solve(const Map& map, const string& algo, const string& heuristic_str, int& iterations) {
    int h_type = 1;
    if (heuristic_str == "H1") h_type = 1;
    else if (heuristic_str == "H2") h_type = 2;
    else if (heuristic_str == "H3") h_type = 3;

    if (algo == "UCS") {
        return runSearch<CmpUCS>(map, h_type, iterations, false, false);
    } else if (algo == "GBFS") {
        return runSearch<CmpGBFS>(map, h_type, iterations, true, true);
    } else { 
        return runSearch<CmpAStar>(map, h_type, iterations, true, false);
    }
}