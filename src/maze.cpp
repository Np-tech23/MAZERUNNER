#include "maze.h"
#include <stack>
#include <queue>
#include <set>
#include <algorithm>

bool Maze::isPerfect() const {
    std::vector<bool> visited(width * height, false);
    std::queue<int> q;
    q.push(0);
    visited[0] = true;
    int reached = 0;
    while (!q.empty()) {
        int pos = q.front();
        q.pop();
        reached++;
        int x = pos % width;
        int y = pos / width;
        for (int d = 0; d < 4; d++) {
            if (!hasWall(x, y, d)) {
                int nx = x + DX[d];
                int ny = y + DY[d];
                if (inBounds(nx, ny)) {
                    int npos = ny * width + nx;
                    if (!visited[npos]) {
                        visited[npos] = true;
                        q.push(npos);
                    }
                }
            }
        }
    }
    int passages = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int val = cells[y * width + x] & 15;
            int walls = 0;
            for(int i=0; i<4; i++) if(val & (1<<i)) walls++;
            passages += (4 - walls);
        }
    }
    passages /= 2;
    return reached == width * height && passages == width * height - 1;
}

std::vector<int> Maze::getAllCells() const {
    std::vector<int> res(width * height);
    for (int i = 0; i < width * height; i++) res[i] = i;
    return res;
}

void generate_DFS(Maze& maze, int seed) {
    std::mt19937 rng(seed);
    std::vector<bool> visited(maze.width * maze.height, false);
    std::stack<int> s;
    int start = std::uniform_int_distribution<int>(0, maze.width * maze.height - 1)(rng);
    visited[start] = true;
    s.push(start);
    while (!s.empty()) {
        int current = s.top();
        int cx = current % maze.width;
        int cy = current / maze.width;
        
        std::vector<int> neighbors;
        for (int d = 0; d < 4; d++) {
            int nx = cx + DX[d];
            int ny = cy + DY[d];
            if (maze.inBounds(nx, ny) && !visited[ny * maze.width + nx]) {
                neighbors.push_back(d);
            }
        }
        
        if (neighbors.empty()) {
            s.pop();
        } else {
            int d = neighbors[std::uniform_int_distribution<int>(0, neighbors.size() - 1)(rng)];
            int nx = cx + DX[d];
            int ny = cy + DY[d];
            int next = ny * maze.width + nx;
            maze.removeWall(cx, cy, d);
            maze.removeWall(nx, ny, OPPOSITE[d]);
            visited[next] = true;
            s.push(next);
        }
    }
}

void generate_Prim(Maze& maze, int seed) {
    std::mt19937 rng(seed);
    std::vector<bool> in_maze(maze.width * maze.height, false);
    struct Wall { int x, y, dir; };
    std::vector<Wall> frontier;
    
    auto add_walls = [&](int x, int y) {
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d];
            int ny = y + DY[d];
            if (maze.inBounds(nx, ny) && !in_maze[ny * maze.width + nx]) {
                frontier.push_back({x, y, d});
            }
        }
    };
    
    int start = std::uniform_int_distribution<int>(0, maze.width * maze.height - 1)(rng);
    int sx = start % maze.width, sy = start / maze.width;
    in_maze[start] = true;
    add_walls(sx, sy);
    
    while (!frontier.empty()) {
        int idx = std::uniform_int_distribution<int>(0, frontier.size() - 1)(rng);
        Wall w = frontier[idx];
        frontier[idx] = frontier.back();
        frontier.pop_back();
        
        int nx = w.x + DX[w.dir];
        int ny = w.y + DY[w.dir];
        int neighbor = ny * maze.width + nx;
        
        if (maze.inBounds(nx, ny) && !in_maze[neighbor]) {
            maze.removeWall(w.x, w.y, w.dir);
            maze.removeWall(nx, ny, OPPOSITE[w.dir]);
            in_maze[neighbor] = true;
            add_walls(nx, ny);
        }
    }
}

void generate_Wilson(Maze& maze, int seed) {
    std::mt19937 rng(seed);
    std::vector<bool> in_maze(maze.width * maze.height, false);
    std::vector<int> next_dir(maze.width * maze.height, -1);
    
    int start = std::uniform_int_distribution<int>(0, maze.width * maze.height - 1)(rng);
    in_maze[start] = true;
    
    int unvisited = maze.width * maze.height - 1;
    
    auto pick_unvisited = [&]() {
        std::vector<int> unv;
        for(int i=0; i<maze.width*maze.height; ++i) if(!in_maze[i]) unv.push_back(i);
        return unv[std::uniform_int_distribution<int>(0, unv.size() - 1)(rng)];
    };
    
    while (unvisited > 0) {
        int current = pick_unvisited();
        int walk_start = current;
        
        while (!in_maze[current]) {
            int cx = current % maze.width;
            int cy = current / maze.width;
            std::vector<int> dirs;
            for(int d=0; d<4; ++d) {
                if (maze.inBounds(cx + DX[d], cy + DY[d])) dirs.push_back(d);
            }
            int d = dirs[std::uniform_int_distribution<int>(0, dirs.size() - 1)(rng)];
            next_dir[current] = d;
            current = (cy + DY[d]) * maze.width + (cx + DX[d]);
        }
        
        current = walk_start;
        while (!in_maze[current]) {
            int d = next_dir[current];
            int cx = current % maze.width;
            int cy = current / maze.width;
            int nx = cx + DX[d];
            int ny = cy + DY[d];
            maze.removeWall(cx, cy, d);
            maze.removeWall(nx, ny, OPPOSITE[d]);
            in_maze[current] = true;
            unvisited--;
            current = ny * maze.width + nx;
        }
    }
}

int Maze::countDeadEnds() const {
    int count = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int open_count = 0;
            for (int d = 0; d < 4; ++d) {
                if (!hasWall(x, y, d)) open_count++;
            }
            if (open_count == 1) count++;
        }
    }
    return count;
}

std::vector<int> Maze::getDeadEnds() const {
    std::vector<int> dead_ends;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int open_count = 0;
            for (int d = 0; d < 4; ++d) {
                if (!hasWall(x, y, d)) open_count++;
            }
            if (open_count == 1) {
                dead_ends.push_back(y * width + x);
            }
        }
    }
    return dead_ends;
}

std::vector<std::string> Maze::toBlockGrid() const {
    int grid_h = 2 * height + 1;
    int grid_w = 2 * width + 1;
    std::vector<std::string> grid(grid_h, std::string(grid_w, '#'));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int gy = 2 * y + 1;
            int gx = 2 * x + 1;
            grid[gy][gx] = ' '; // Open cell center

            if (!hasWall(x, y, NORTH)) grid[gy - 1][gx] = ' ';
            if (!hasWall(x, y, SOUTH)) grid[gy + 1][gx] = ' ';
            if (!hasWall(x, y, WEST))  grid[gy][gx - 1] = ' ';
            if (!hasWall(x, y, EAST))  grid[gy][gx + 1] = ' ';
        }
    }
    return grid;
}

