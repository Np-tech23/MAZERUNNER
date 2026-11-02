#pragma once
#include <random>
#include <string>
#include <vector>

// Directions. The order matters: OPPOSITE[d] is the reverse of d.
enum Dir { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 };
constexpr int DX[4]       = { 0, 1, 0, -1 };
constexpr int DY[4]       = { -1, 0, 1, 0 };
constexpr int OPPOSITE[4] = { SOUTH, WEST, NORTH, EAST };

// A "perfect" maze: exactly one path between any two cells (no loops).
// Each cell stores 4 bits; bit d set means "wall present on side d".
class Maze {
public:
    Maze(int width, int height);

    int width() const  { return w_; }
    int height() const { return h_; }
    bool inBounds(int x, int y) const { return x >= 0 && y >= 0 && x < w_ && y < h_; }
    bool hasWall(int x, int y, int d) const { return walls_[y * w_ + x] & (1 << d); }

    void reset();                      // put all walls back
    void generateDFS(std::mt19937& rng);
    void generatePrim(std::mt19937& rng);
    void generateWilson(std::mt19937& rng);

    // Metrics / testing
    bool isPerfect() const;            // connected + no loops
    int  countDeadEnds() const;        // cells with exactly 1 opening

    // Converts to a grid of '#' (wall) and ' ' (open), size (2h+1) x (2w+1).
    // This is the format the 3D renderer will use later.
    std::vector<std::string> toBlockGrid() const;
    void print() const;

private:
    void carve(int x, int y, int d);   // remove wall between (x,y) and its neighbour
    int  openings(int x, int y) const;

    int w_, h_;
    std::vector<unsigned char> walls_;
};
