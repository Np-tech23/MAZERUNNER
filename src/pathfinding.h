#pragma once
#include "maze.h"
#include <vector>

struct PathfindingStats {
    std::vector<int> path;
    int nodes_expanded = 0;
    long long runtime_us = 0;
};

PathfindingStats bfs(const Maze& maze, int start, int goal);
PathfindingStats astar(const Maze& maze, int start, int goal);
PathfindingStats bidirectional_bfs(const Maze& maze, int start, int goal);
