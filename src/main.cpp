#include "maze.h"
#include "pathfinding.h"
#include "player.h"
#include "monster.h"
#include "database.h"
#include "graphics.h"
#include "game.h"
#include "tests.h"
#include <iostream>
#include <string>
#include <iomanip>
#include <chrono>

using namespace std::chrono;

static void print_ascii_maze(const Maze& maze, int start, int goal, const std::vector<int>& path = {}) {
    std::vector<std::string> grid = maze.toBlockGrid();
    int h = static_cast<int>(grid.size());
    int w = static_cast<int>(grid[0].size());

    // Mark path
    for (int node : path) {
        int cx = node % maze.width;
        int cy = node / maze.width;
        grid[2 * cy + 1][2 * cx + 1] = '*';
    }

    // Mark start & exit
    int sx = start % maze.width, sy = start / maze.width;
    int gx = goal % maze.width, gy = goal / maze.width;
    grid[2 * sy + 1][2 * sx + 1] = 'S';
    grid[2 * gy + 1][2 * gx + 1] = 'E';

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            std::cout << grid[y][x] << " ";
        }
        std::cout << "\n";
    }
}

static void run_benchmarks() {
    std::cout << "\n==================== MAZERUNNER PERFORMANCE BENCHMARK ====================\n";
    std::cout << std::left << std::setw(14) << "Algorithm" 
              << std::setw(12) << "Maze Size" 
              << std::setw(16) << "Runtime (us)" 
              << std::setw(18) << "Nodes / Passages" 
              << std::setw(12) << "Optimality" << "\n";
    std::cout << std::string(72, '-') << "\n";

    int sizes[] = {15, 30, 50};
    int seed = 42;

    for (int s : sizes) {
        // DFS
        Maze m1(s, s);
        auto t1 = high_resolution_clock::now();
        generate_DFS(m1, seed);
        auto t2 = high_resolution_clock::now();
        std::cout << std::left << std::setw(14) << "DFS"
                  << std::setw(12) << (std::to_string(s) + "x" + std::to_string(s))
                  << std::setw(16) << duration_cast<microseconds>(t2 - t1).count()
                  << std::setw(18) << (s * s - 1)
                  << std::setw(12) << (m1.isPerfect() ? "PERFECT" : "ERROR") << "\n";

        // Prim
        Maze m2(s, s);
        t1 = high_resolution_clock::now();
        generate_Prim(m2, seed);
        t2 = high_resolution_clock::now();
        std::cout << std::left << std::setw(14) << "Prim"
                  << std::setw(12) << (std::to_string(s) + "x" + std::to_string(s))
                  << std::setw(16) << duration_cast<microseconds>(t2 - t1).count()
                  << std::setw(18) << (s * s - 1)
                  << std::setw(12) << (m2.isPerfect() ? "PERFECT" : "ERROR") << "\n";

        // Wilson
        Maze m3(s, s);
        t1 = high_resolution_clock::now();
        generate_Wilson(m3, seed);
        t2 = high_resolution_clock::now();
        std::cout << std::left << std::setw(14) << "Wilson"
                  << std::setw(12) << (std::to_string(s) + "x" + std::to_string(s))
                  << std::setw(16) << duration_cast<microseconds>(t2 - t1).count()
                  << std::setw(18) << (s * s - 1)
                  << std::setw(12) << (m3.isPerfect() ? "PERFECT" : "ERROR") << "\n";
    }

    std::cout << "\n--- Pathfinding Solvers Comparison (30x30 Maze, Seed 42) ---\n";
    Maze m_sol(30, 30);
    generate_DFS(m_sol, 42);
    int start = 0, goal = 30 * 30 - 1;

    PathfindingStats bs = bfs(m_sol, start, goal);
    std::cout << std::left << std::setw(14) << "BFS"
              << std::setw(12) << "30x30"
              << std::setw(16) << bs.runtime_us
              << std::setw(18) << (std::to_string(bs.nodes_expanded) + " nodes")
              << std::setw(12) << ("Length: " + std::to_string(bs.path.size())) << "\n";

    PathfindingStats as = astar(m_sol, start, goal);
    std::cout << std::left << std::setw(14) << "A* (Euclidean)"
              << std::setw(12) << "30x30"
              << std::setw(16) << as.runtime_us
              << std::setw(18) << (std::to_string(as.nodes_expanded) + " nodes")
              << std::setw(12) << ("Length: " + std::to_string(as.path.size())) << "\n";

    PathfindingStats bbs = bidirectional_bfs(m_sol, start, goal);
    std::cout << std::left << std::setw(14) << "Bi-BFS"
              << std::setw(12) << "30x30"
              << std::setw(16) << bbs.runtime_us
              << std::setw(18) << (std::to_string(bbs.nodes_expanded) + " nodes")
              << std::setw(12) << ("Length: " + std::to_string(bbs.path.size())) << "\n";

    std::cout << "==========================================================================\n\n";
}

int main(int argc, char* argv[]) {
    // Command line argument flags for automation
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--test" || arg == "-t") {
            bool ok = runAllTests();
            return ok ? 0 : 1;
        } else if (arg == "--benchmark" || arg == "-b") {
            run_benchmarks();
            return 0;
        } else if (arg == "--db") {
            Database db;
            db.open("data/mazerunner.db");
            db.printLeaderboard(10);
            return 0;
        } else if (arg == "--3d") {
            Game game;
            game.init(15, 10, 42, "STANDARD");
            game.run();
            return 0;
        }
    }

    std::cout << "\n============================================================\n";
    std::cout << "          MAZERUNNER 3D — FULL MILESTONE 5 RELEASE          \n";
    std::cout << "    MITS Gwalior • B.Tech CSE Macro Project-I (15252110)    \n";
    std::cout << "============================================================\n";

    Database db;
    db.open("data/mazerunner.db");

    while (true) {
        std::cout << "\n=== MAIN MENU ===\n";
        std::cout << "1) Launch 3D MazeRunner Game (OpenGL 3D First-Person)\n";
        std::cout << "2) Run Automated Milestone 1-5 Verification Test Suite\n";
        std::cout << "3) View Leaderboard & Player Sessions (SQLite3)\n";
        std::cout << "4) Run Algorithmic Performance Benchmarks\n";
        std::cout << "5) Generate & Solve ASCII Terminal Maze\n";
        std::cout << "0) Exit\n";
        std::cout << "Select Option: ";

        int choice;
        if (!(std::cin >> choice)) break;

        if (choice == 0) {
            std::cout << "Exiting MazeRunner. Goodbye!\n";
            break;
        } else if (choice == 1) {
            int w = 15, h = 10, mode_choice = 1;
            std::cout << "\nSelect Game Mode:\n";
            std::cout << "  1) Standard Exploration (3 Keys, 2 Monsters)\n";
            std::cout << "  2) Time Trial (5-Minute Countdown, 3 Monsters)\n";
            std::cout << "  3) Survival Mode (5 Monsters, High Threat)\n";
            std::cout << "Mode (1-3): "; std::cin >> mode_choice;

            std::cout << "Maze Dimensions (Width Height, e.g. 15 10): "; std::cin >> w >> h;
            if (w < 5) w = 15;
            if (h < 5) h = 10;

            std::string mode_str = (mode_choice == 2) ? "TIME_TRIAL" : ((mode_choice == 3) ? "SURVIVAL" : "STANDARD");
            int seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count() % 100000);

            std::cout << "\nStarting 3D Game Window...\n";
            std::cout << "Controls:\n";
            std::cout << "  WASD: Move | Mouse: Look | Shift: Sprint\n";
            std::cout << "  F: Toggle Flashlight | H: Toggle A* Path Breadcrumbs\n";
            std::cout << "  F5: Quick Save | F9: Quick Load | ESC: Pause\n\n";

            Game game;
            if (game.init(w, h, seed, mode_str)) {
                game.run();
            }
        } else if (choice == 2) {
            runAllTests();
        } else if (choice == 3) {
            db.printLeaderboard(10);
        } else if (choice == 4) {
            run_benchmarks();
        } else if (choice == 5) {
            int algo = 1, w = 12, h = 8;
            std::cout << "Algorithm (1=DFS, 2=Prim, 3=Wilson): "; std::cin >> algo;
            std::cout << "Width Height (e.g. 12 8): "; std::cin >> w >> h;
            Maze m(w, h);
            int seed = 42;
            if (algo == 1) generate_DFS(m, seed);
            else if (algo == 2) generate_Prim(m, seed);
            else if (algo == 3) generate_Wilson(m, seed);

            std::cout << "Generated " << (algo==1?"DFS":(algo==2?"Prim":"Wilson")) << " Maze (" << w << "x" << h << "). Perfect: " << (m.isPerfect()?"YES":"NO") << "\n";
            int start = 0, goal = w * h - 1;
            PathfindingStats s = astar(m, start, goal);
            std::cout << "A* Shortest Path: " << s.path.size() << " steps (" << s.nodes_expanded << " nodes expanded)\n\n";
            print_ascii_maze(m, start, goal, s.path);
        }
    }

    return 0;
}
