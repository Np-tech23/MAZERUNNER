#pragma once
#include <vector>
#include <cstdint>
#include <random>

enum Dir { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 };
constexpr int DX[4] = {0, 1, 0, -1};
constexpr int DY[4] = {-1, 0, 1, 0};
constexpr int OPPOSITE[4] = {SOUTH, WEST, NORTH, EAST};

struct Maze {
    int width, height;
    std::vector<uint8_t> cells; // 4 bits per cell: [3:W][2:S][1:E][0:N]

    Maze(int w, int h) : width(w), height(h), cells(w * h, 15) {}

    bool inBounds(int x, int y) const {
        return x >= 0 && x < width && y >= 0 && y < height;
    }

    bool hasWall(int x, int y, int dir) const {
        return (cells[y * width + x] & (1 << dir)) != 0;
    }

    void removeWall(int x, int y, int dir) {
        cells[y * width + x] &= ~(1 << dir);
    }
    
    void addWall(int x, int y, int dir) {
        cells[y * width + x] |= (1 << dir);
    }

    bool isPerfect() const;
    std::vector<int> getAllCells() const;
    int countDeadEnds() const;
    std::vector<int> getDeadEnds() const;
    std::vector<std::string> toBlockGrid() const;
};

void generate_DFS(Maze& maze, int seed);
void generate_Prim(Maze& maze, int seed);
void generate_Wilson(Maze& maze, int seed);
