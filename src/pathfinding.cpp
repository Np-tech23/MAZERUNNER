#include "pathfinding.h"
#include <queue>
#include <set>
#include <unordered_set>
#include <chrono>
#include <cmath>
#include <algorithm>

using namespace std::chrono;

int manhattan_h(int pos, int goal, int width) {
    int x1 = pos % width, y1 = pos / width;
    int x2 = goal % width, y2 = goal / width;
    return static_cast<int>(std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)) * 0.9);
}

std::vector<int> reconstruct_path(const std::vector<int>& parent, int start, int goal) {
    std::vector<int> path;
    int current = goal;
    while (current != start) {
        path.push_back(current);
        current = parent[current];
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<int> reconstruct_bidirectional_path(const std::vector<int>& f_parent, const std::vector<int>& b_parent, int current, int neighbor, int start, int goal) {
    std::vector<int> f_path, b_path;
    int node = current;
    while(node != start) {
        f_path.push_back(node);
        node = f_parent[node];
    }
    f_path.push_back(start);
    std::reverse(f_path.begin(), f_path.end());
    
    node = neighbor;
    while(node != goal) {
        b_path.push_back(node);
        node = b_parent[node];
    }
    b_path.push_back(goal);
    
    f_path.insert(f_path.end(), b_path.begin(), b_path.end());
    return f_path;
}

std::vector<int> get_neighbors(const Maze& maze, int current) {
    std::vector<int> neighbors;
    int x = current % maze.width;
    int y = current / maze.width;
    for (int d = 0; d < 4; d++) {
        if (!maze.hasWall(x, y, d)) {
            int nx = x + DX[d];
            int ny = y + DY[d];
            if (maze.inBounds(nx, ny)) {
                neighbors.push_back(ny * maze.width + nx);
            }
        }
    }
    return neighbors;
}

PathfindingStats bfs(const Maze& maze, int start, int goal) {
    PathfindingStats stats;
    auto start_time = high_resolution_clock::now();
    
    std::queue<int> q;
    std::vector<bool> visited(maze.width * maze.height, false);
    std::vector<int> parent(maze.width * maze.height, -1);
    
    q.push(start);
    visited[start] = true;
    
    while (!q.empty()) {
        int current = q.front();
        q.pop();
        stats.nodes_expanded++;
        
        if (current == goal) {
            stats.path = reconstruct_path(parent, start, goal);
            break;
        }
        
        for (int neighbor : get_neighbors(maze, current)) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                parent[neighbor] = current;
                q.push(neighbor);
            }
        }
    }
    
    stats.runtime_us = duration_cast<microseconds>(high_resolution_clock::now() - start_time).count();
    return stats;
}

PathfindingStats astar(const Maze& maze, int start, int goal) {
    PathfindingStats stats;
    auto start_time = high_resolution_clock::now();
    
    std::vector<int> g_score(maze.width * maze.height, 1e9);
    std::vector<int> h_score(maze.width * maze.height, 1e9);
    std::vector<int> parent(maze.width * maze.height, -1);
    
    g_score[start] = 0;
    h_score[start] = manhattan_h(start, goal, maze.width);
    
    auto cmp = [&](int a, int b) {
        int fa = g_score[a] + h_score[a];
        int fb = g_score[b] + h_score[b];
        if (fa == fb) return h_score[a] > h_score[b];
        return fa > fb;
    };
    std::priority_queue<int, std::vector<int>, decltype(cmp)> open_set(cmp);
    std::vector<bool> in_open(maze.width * maze.height, false);
    std::vector<bool> closed_set(maze.width * maze.height, false);
    
    open_set.push(start);
    in_open[start] = true;
    
    while (!open_set.empty()) {
        int current = open_set.top();
        open_set.pop();
        in_open[current] = false;
        
        if (closed_set[current]) continue;
        closed_set[current] = true;
        stats.nodes_expanded++;
        
        if (current == goal) {
            stats.path = reconstruct_path(parent, start, goal);
            break;
        }
        
        for (int neighbor : get_neighbors(maze, current)) {
            if (closed_set[neighbor]) continue;
            
            int tentative_g = g_score[current] + 1; // distance is 1
            if (tentative_g < g_score[neighbor]) {
                g_score[neighbor] = tentative_g;
                h_score[neighbor] = manhattan_h(neighbor, goal, maze.width);
                parent[neighbor] = current;
                
                if (!in_open[neighbor]) {
                    open_set.push(neighbor);
                    in_open[neighbor] = true;
                } else {
                    // Update priority
                    open_set.push(neighbor);
                }
            }
        }
    }
    
    stats.runtime_us = duration_cast<microseconds>(high_resolution_clock::now() - start_time).count();
    return stats;
}

PathfindingStats bidirectional_bfs(const Maze& maze, int start, int goal) {
    PathfindingStats stats;
    auto start_time = high_resolution_clock::now();
    
    if (start == goal) {
        stats.path = {start};
        stats.runtime_us = duration_cast<microseconds>(high_resolution_clock::now() - start_time).count();
        return stats;
    }
    
    std::queue<int> f_q, b_q;
    std::vector<int> f_parent(maze.width * maze.height, -1);
    std::vector<int> b_parent(maze.width * maze.height, -1);
    std::vector<bool> f_visited(maze.width * maze.height, false);
    std::vector<bool> b_visited(maze.width * maze.height, false);
    
    f_q.push(start);
    f_visited[start] = true;
    b_q.push(goal);
    b_visited[goal] = true;
    
    while (!f_q.empty() && !b_q.empty()) {
        int f_curr = f_q.front();
        f_q.pop();
        stats.nodes_expanded++;
        
        for (int neighbor : get_neighbors(maze, f_curr)) {
            if (b_visited[neighbor]) {
                stats.path = reconstruct_bidirectional_path(f_parent, b_parent, f_curr, neighbor, start, goal);
                goto end_bfs;
            }
            if (!f_visited[neighbor]) {
                f_visited[neighbor] = true;
                f_parent[neighbor] = f_curr;
                f_q.push(neighbor);
            }
        }
        
        int b_curr = b_q.front();
        b_q.pop();
        stats.nodes_expanded++;
        
        for (int neighbor : get_neighbors(maze, b_curr)) {
            if (f_visited[neighbor]) {
                stats.path = reconstruct_bidirectional_path(f_parent, b_parent, neighbor, b_curr, start, goal);
                goto end_bfs;
            }
            if (!b_visited[neighbor]) {
                b_visited[neighbor] = true;
                b_parent[neighbor] = b_curr;
                b_q.push(neighbor);
            }
        }
    }
end_bfs:
    stats.runtime_us = duration_cast<microseconds>(high_resolution_clock::now() - start_time).count();
    return stats;
}
