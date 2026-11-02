#include "maze.hpp"
#include <chrono>
#include <iostream>
#include <string>

static void runOne(const char* name, int w, int h, unsigned seed,
                   void (Maze::*gen)(std::mt19937&), bool show) {
    Maze m(w, h);
    std::mt19937 rng(seed);
    auto t0 = std::chrono::steady_clock::now();
    (m.*gen)(rng);
    auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();

    if (show) m.print();
    std::cout << name << ": " << ms << " ms | dead ends: " << m.countDeadEnds()
              << " | perfect: " << (m.isPerfect() ? "YES" : "NO  <-- BUG") << "\n";
}

int main() {
    int w = 15, h = 10;
    unsigned seed = std::random_device{}();

    while (true) {
        std::cout << "\n=== MazeRunner - Week 1 ===\n"
                  << "Size: " << w << " x " << h << "   Seed: " << seed << "\n"
                  << "1) DFS   2) Prim   3) Wilson   4) Compare all   5) Change size\n"
                  << "6) Set seed   0) Quit\n> ";
        int c;
        if (!(std::cin >> c)) break;
        if (c == 0) break;

        switch (c) {
            case 1: runOne("DFS",    w, h, seed, &Maze::generateDFS,    true); break;
            case 2: runOne("Prim",   w, h, seed, &Maze::generatePrim,   true); break;
            case 3: runOne("Wilson", w, h, seed, &Maze::generateWilson, true); break;
            case 4:
                runOne("DFS   ", w, h, seed, &Maze::generateDFS,    false);
                runOne("Prim  ", w, h, seed, &Maze::generatePrim,   false);
                runOne("Wilson", w, h, seed, &Maze::generateWilson, false);
                break;
            case 5:
                std::cout << "Width Height (2-100): ";
                std::cin >> w >> h;
                if (w < 2 || h < 2 || w > 100 || h > 100) { w = 15; h = 10; std::cout << "Reset to 15x10\n"; }
                break;
            case 6:
                std::cout << "Seed: ";
                std::cin >> seed;
                break;
            default: std::cout << "Unknown option.\n";
        }
    }
    return 0;
}
