#include "maze.hpp"
#include <iostream>
#include <string>
#include <utility>

Maze::Maze(int width, int height) : w_(width), h_(height) { reset(); }

void Maze::reset() { walls_.assign(w_ * h_, 0b1111); }

void Maze::carve(int x, int y, int d) {
    int nx = x + DX[d], ny = y + DY[d];
    walls_[y * w_ + x]   &= ~(1 << d);
    walls_[ny * w_ + nx] &= ~(1 << OPPOSITE[d]);
}

// ---------- Algorithm 1: Depth-First Search (recursive backtracker) ----------
// Long, winding corridors, few branches. Iterative version (own stack)
// so big mazes can't overflow the call stack.
void Maze::generateDFS(std::mt19937& rng) {
    reset();
    std::vector<bool> visited(w_ * h_, false);
    std::vector<std::pair<int,int>> stack;

    int sx = std::uniform_int_distribution<int>(0, w_ - 1)(rng);
    int sy = std::uniform_int_distribution<int>(0, h_ - 1)(rng);
    visited[sy * w_ + sx] = true;
    stack.push_back({sx, sy});

    while (!stack.empty()) {
        auto [x, y] = stack.back();
        int options[4], n = 0;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (inBounds(nx, ny) && !visited[ny * w_ + nx]) options[n++] = d;
        }
        if (n == 0) { stack.pop_back(); continue; }        // dead end: backtrack
        int d = options[std::uniform_int_distribution<int>(0, n - 1)(rng)];
        carve(x, y, d);
        visited[(y + DY[d]) * w_ + (x + DX[d])] = true;
        stack.push_back({x + DX[d], y + DY[d]});
    }
}

// ---------- Algorithm 2: Randomized Prim's ----------
// Grow from one cell; keep a list of "frontier walls" and pick one at random.
// Many short branches, feels more like a tangled web.
void Maze::generatePrim(std::mt19937& rng) {
    reset();
    struct Wall { int x, y, d; };
    std::vector<bool> inMaze(w_ * h_, false);
    std::vector<Wall> frontier;

    auto addWalls = [&](int x, int y) {
        inMaze[y * w_ + x] = true;
        for (int d = 0; d < 4; d++)
            if (inBounds(x + DX[d], y + DY[d]) && !inMaze[(y + DY[d]) * w_ + (x + DX[d])])
                frontier.push_back({x, y, d});
    };

    addWalls(std::uniform_int_distribution<int>(0, w_ - 1)(rng),
             std::uniform_int_distribution<int>(0, h_ - 1)(rng));

    while (!frontier.empty()) {
        size_t i = std::uniform_int_distribution<size_t>(0, frontier.size() - 1)(rng);
        Wall w = frontier[i];
        frontier[i] = frontier.back();                     // O(1) removal
        frontier.pop_back();

        int nx = w.x + DX[w.d], ny = w.y + DY[w.d];
        if (inMaze[ny * w_ + nx]) continue;                // would create a loop
        carve(w.x, w.y, w.d);
        addWalls(nx, ny);
    }
}

// ---------- Algorithm 3: Wilson's (loop-erased random walk) ----------
// Produces *unbiased* mazes (every possible maze equally likely).
// Start a random walk from an unvisited cell until it hits the maze;
// erase any loops it made; add the path. Slow at first, fast at the end.
void Maze::generateWilson(std::mt19937& rng) {
    reset();
    const int total = w_ * h_;
    std::vector<bool> inMaze(total, false);
    std::vector<int> nextDir(total, -1);

    inMaze[std::uniform_int_distribution<int>(0, total - 1)(rng)] = true;
    int remaining = total - 1;
    std::uniform_int_distribution<int> anyCell(0, total - 1), anyDir(0, 3);

    while (remaining > 0) {
        int start;
        do { start = anyCell(rng); } while (inMaze[start]);

        // Random walk. Overwriting nextDir[] on revisits IS the loop erasure.
        int cur = start;
        while (!inMaze[cur]) {
            int x = cur % w_, y = cur / w_, d;
            do { d = anyDir(rng); } while (!inBounds(x + DX[d], y + DY[d]));
            nextDir[cur] = d;
            cur = (y + DY[d]) * w_ + (x + DX[d]);
        }
        // Retrace the loop-free path and carve it.
        cur = start;
        while (!inMaze[cur]) {
            int x = cur % w_, y = cur / w_, d = nextDir[cur];
            carve(x, y, d);
            inMaze[cur] = true;
            remaining--;
            cur = (y + DY[d]) * w_ + (x + DX[d]);
        }
    }
}

// ---------- Metrics ----------
int Maze::openings(int x, int y) const {
    int n = 0;
    for (int d = 0; d < 4; d++) if (!hasWall(x, y, d)) n++;
    return n;
}

int Maze::countDeadEnds() const {
    int n = 0;
    for (int y = 0; y < h_; y++)
        for (int x = 0; x < w_; x++)
            if (openings(x, y) == 1) n++;
    return n;
}

// A maze is perfect if every cell is reachable from (0,0) AND
// passages == cells - 1 (a spanning tree: connected, no loops).
bool Maze::isPerfect() const {
    std::vector<bool> seen(w_ * h_, false);
    std::vector<std::pair<int,int>> st{{0, 0}};
    seen[0] = true;
    int reached = 0;
    while (!st.empty()) {
        auto [x, y] = st.back(); st.pop_back();
        reached++;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!hasWall(x, y, d) && inBounds(nx, ny) && !seen[ny * w_ + nx]) {
                seen[ny * w_ + nx] = true;
                st.push_back({nx, ny});
            }
        }
    }
    int passages = 0;
    for (int y = 0; y < h_; y++)
        for (int x = 0; x < w_; x++)
            passages += openings(x, y);
    passages /= 2;                                         // each counted twice
    return reached == w_ * h_ && passages == w_ * h_ - 1;
}

// ---------- Output ----------
std::vector<std::string> Maze::toBlockGrid() const {
    std::vector<std::string> g(2 * h_ + 1, std::string(2 * w_ + 1, '#'));
    for (int y = 0; y < h_; y++)
        for (int x = 0; x < w_; x++) {
            int gx = 2 * x + 1, gy = 2 * y + 1;
            g[gy][gx] = ' ';
            if (!hasWall(x, y, EAST))  g[gy][gx + 1] = ' ';
            if (!hasWall(x, y, SOUTH)) g[gy + 1][gx] = ' ';
        }
    return g;
}

void Maze::print() const {
    auto g = toBlockGrid();
    g[1][1] = 'S';                                         // start
    g[2 * h_ - 1][2 * w_ - 1] = 'E';                       // exit
    for (auto& row : g) {
        for (char c : row) std::cout << c << c;            // double width: terminal chars are tall
        std::cout << '\n';
    }
}
