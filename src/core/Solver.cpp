#include "Solver.hpp"
#include <queue>
#include <cmath>
#include <limits>

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

static inline bool isGoalState(const Map& map, const State& state) {
    return state.mask == map.total_waypoints && map.grid[state.r][state.c] == 'O';
}

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

Node runBFS(const Map& map, int& iterations, vector<IterationLog>* iteration_logs) {
    queue<Node> q;
    vector<vector<vector<bool>>> visited(
        map.rows, vector<vector<bool>>(map.cols, vector<bool>(map.total_waypoints + 1, false))
    );

    Node start_node;
    start_node.state = {map.start.r, map.start.c, 0};
    start_node.g = 0;
    start_node.h = 0;
    start_node.path = "";

    q.push(start_node);
    visited[start_node.state.r][start_node.state.c][0] = true;
    iterations = 0;
    const char dirs[] = {'U', 'D', 'L', 'R'};

    while (!q.empty()) {
        Node curr = q.front();
        q.pop();

        iterations++;
        if (iteration_logs != nullptr) {
            iteration_logs->push_back({iterations, curr.state, curr.g, curr.h, curr.path});
        }

        if (isGoalState(map, curr.state)) {
            return curr;
        }

        for (char dir : dirs) {
            SlideResult res = performSlide(map, curr.state, dir);
            if (!res.valid) continue;

            State ns = res.final_state;
            if (visited[ns.r][ns.c][ns.mask]) continue;
            visited[ns.r][ns.c][ns.mask] = true;

            Node next_node;
            next_node.state = ns;
            next_node.g = curr.g + res.cost_added;
            next_node.h = 0;
            next_node.path = curr.path + dir;
            q.push(next_node);
        }
    }

    return Node{{-1, -1, -1}, -1, -1, ""};
}

double idaDfs(
    const Map& map,
    int h_type,
    State state,
    int g,
    double threshold,
    string& path,
    vector<int>& best_g,
    int& iterations,
    vector<IterationLog>* iteration_logs,
    Node& found
) {
    const double h = computeHeuristic(h_type, map, state);
    const double f = static_cast<double>(g) + h;
    if (f > threshold) return f;

    const int idx = (state.r * map.cols + state.c) * (map.total_waypoints + 1) + state.mask;
    if (g >= best_g[idx]) {
        return numeric_limits<double>::infinity();
    }
    best_g[idx] = g;

    iterations++;
    if (iteration_logs != nullptr) {
        iteration_logs->push_back({iterations, state, g, h, path});
    }

    if (isGoalState(map, state)) {
        found = Node{state, g, h, path};
        return -1.0; // found marker
    }

    double min_next = numeric_limits<double>::infinity();
    const char dirs[] = {'U', 'D', 'L', 'R'};
    for (char dir : dirs) {
        SlideResult res = performSlide(map, state, dir);
        if (!res.valid) continue;

        path.push_back(dir);
        double t = idaDfs(
            map,
            h_type,
            res.final_state,
            g + res.cost_added,
            threshold,
            path,
            best_g,
            iterations,
            iteration_logs,
            found
        );
        path.pop_back();

        if (t < 0.0) return -1.0;
        if (t < min_next) min_next = t;
    }

    return min_next;
}

Node runIDAStar(
    const Map& map,
    int h_type,
    int& iterations,
    vector<IterationLog>* iteration_logs
) {
    iterations = 0;
    State start{map.start.r, map.start.c, 0};
    double threshold = computeHeuristic(h_type, map, start);
    Node found{{-1, -1, -1}, -1, -1, ""};

    const int total_states = map.rows * map.cols * (map.total_waypoints + 1);
    while (true) {
        vector<int> best_g(total_states, numeric_limits<int>::max());
        string path;

        double t = idaDfs(
            map,
            h_type,
            start,
            0,
            threshold,
            path,
            best_g,
            iterations,
            iteration_logs,
            found
        );

        if (t < 0.0) {
            return found;
        }
        if (!isfinite(t)) {
            return Node{{-1, -1, -1}, -1, -1, ""};
        }
        if (t <= threshold) {
            threshold += 1.0;
        } else {
            threshold = t;
        }
    }
}

template <typename Cmp>
Node runSearch(
    const Map& map,
    int h_type,
    int& iterations,
    bool use_h,
    bool is_gbfs,
    vector<IterationLog>* iteration_logs
) {
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
        if (iteration_logs != nullptr) {
            iteration_logs->push_back({iterations, curr.state, curr.g, curr.h, curr.path});
        }

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

Node solve(
    const Map& map,
    const string& algo,
    const string& heuristic_str,
    int& iterations,
    vector<IterationLog>* iteration_logs
) {
    int h_type = 1;
    if (heuristic_str == "H1") h_type = 1;
    else if (heuristic_str == "H2") h_type = 2;
    else if (heuristic_str == "H3") h_type = 3;

    if (algo == "UCS") {
        return runSearch<CmpUCS>(map, h_type, iterations, false, false, iteration_logs);
    } else if (algo == "BFS") {
        return runBFS(map, iterations, iteration_logs);
    } else if (algo == "GBFS") {
        return runSearch<CmpGBFS>(map, h_type, iterations, true, true, iteration_logs);
    } else if (algo == "IDA*") {
        return runIDAStar(map, h_type, iterations, iteration_logs);
    } else { 
        return runSearch<CmpAStar>(map, h_type, iterations, true, false, iteration_logs);
    }
}