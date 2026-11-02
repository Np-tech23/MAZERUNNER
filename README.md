# MazeRunner 3D — Real-Time Labyrinth Simulation & Survival Game

**Academic Project**: B.Tech Computer Science & Engineering (Semester III)  
**Course**: Macro Project-I (Subject Code: 15252110)  
**Institution**: Madhav Institute of Technology & Science (MITS), Gwalior  
**Author**: Niyati Patel (Roll: `BTCS25O1093`)  
**Faculty Supervisor**: Mrs. Manisha Pathak (Assistant Professor, Dept. of CSE)  

---

## Project Overview

**MazeRunner** is a hardware-accelerated 3D first-person maze exploration and survival game written in modern **C++17**, powered by **OpenGL 3.3 Core Profile**, **GLFW**, **GLEW**, **GLM**, and an embedded **SQLite 3** persistence engine. 

The project incorporates complete procedural maze generation, optimal pathfinding algorithms, autonomous enemy AI with Finite State Machines (FSM), interactive 3-key objective loops, dynamic Blinn-Phong illumination with player spotlights, and persistent save slots with real-time leaderboard analytics.

---

## Architecture & Subsystems (Milestones 1 – 5)

| Subsystem | Source Files | Features & Implementation Details |
| :--- | :--- | :--- |
| **Maze Topology** | `src/maze.h`, `src/maze.cpp` | • **Generation**: DFS Recursive Backtracker, Randomized Prim's, Wilson's Loop-Erased Random Walk.<br>• **Validation**: `isPerfect()` spanning tree proof, `countDeadEnds()`, and `toBlockGrid()` 3D coordinate mapping. |
| **Pathfinding** | `src/pathfinding.h`, `src/pathfinding.cpp` | • **Algorithms**: BFS (Breadth-First Search), A* Search (admissible Euclidean heuristic), and Bidirectional BFS.<br>• **Metrics**: Step counting, node expansion profiling, and microsecond runtime benchmarks. |
| **Player Controller** | `src/player.h`, `src/player.cpp` | • **6-DOF Camera**: Euler-angle first-person navigation, mouse look delta integration.<br>• **Physics**: Axis-Aligned Bounding Box (AABB) with tangent wall sliding.<br>• **Mechanics**: Stamina-based sprint, health system, toggleable torchlight, and 3-key inventory. |
| **Autonomous AI** | `src/monster.h`, `src/monster.cpp` | • **State Machine**: `PATROL` (wandering), `DETECT` (10-unit / 110° FOV raycast LOS), `CHASE` (dynamic A* pursuit), `SEARCH` (investigating last known pos).<br>• **Combat**: Proximity damage delivery (25 HP) with anti-stuck clearance checks. |
| **Persistence & DB** | `src/database.h`, `src/database.cpp` | • **SQLite3 WAL Mode**: High-throughput non-blocking writes (`PRAGMA journal_mode=WAL;`).<br>• **Saves**: 3 independent save slots (`save_slots`) with atomic transactions.<br>• **Leaderboards**: SQL views (`v_leaderboard`) tracking player ranking, score, and escape time. |
| **Graphics Engine** | `src/graphics.h`, `src/graphics.cpp` | • **OpenGL 3.3 Core Profile**: Vertex & fragment GLSL shaders.<br>• **Lighting & Shading**: Blinn-Phong dynamic player spotlight, ambient fill, distance exponential fog.<br>• **Entities & HUD**: Rotating 3D key gems, exit portal, red monster avatars, glowing breadcrumb hint trail (`H`), minimap overlay, stamina & health bars. |
| **Game Engine** | `src/game.h`, `src/game.cpp` | • **Game Modes**: Standard Exploration, Time Trial (5 min limit), Survival Mode.<br>• **Lifecycle**: Win/Loss conditions, pause menu, runtime telemetry, and slot save/load. |
| **Verification Suite**| `src/tests.h`, `src/tests.cpp` | • **Automated Tests**: 19 test cases covering all 5 milestones with 100% pass verification. |

---

## Build Instructions

### Prerequisites
- Windows 10/11 x64
- MSYS2 with UCRT64 toolchain (`gcc 15.x`, `cmake`, `ninja`, `glfw3`, `glew`, `glm`, `sqlite3`)

### Option A: Building with CMake & Ninja (Recommended)
```powershell
# Set MSYS2 UCRT64 path in PowerShell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH

# Configure with CMake
cmake -S . -B build -G Ninja

# Compile Release binary
cmake --build build --config Release
```

### Option B: Direct Compilation with g++
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH

g++ -std=c++17 -Isrc src/*.cpp -lglfw3 -lglew32 -lopengl32 -lsqlite3 -lgdi32 -O3 -o mazerunner.exe
```

---

## Running the Application

### 1. Interactive 3D Game & Console Menu
```powershell
.\mazerunner.exe
```
This presents an interactive menu allowing you to launch the 3D game, view leaderboards, run benchmarks, or solve ASCII mazes.

### 2. Direct 3D Game Launch
```powershell
.\mazerunner.exe --3d
```

### 3. Automated Milestone Verification Tests
```powershell
.\mazerunner.exe --test
```

### 4. Algorithmic Benchmarks
```powershell
.\mazerunner.exe --benchmark
```

### 5. Leaderboard Inspection
```powershell
.\mazerunner.exe --db
```

---

## In-Game Controls

| Key / Input | Action |
| :--- | :--- |
| **W, A, S, D** | Move forward, strafe left, move backward, strafe right |
| **Mouse Delta** | 3D Camera Look (Pitch & Yaw) |
| **Left Shift** | Sprint (drains stamina, moves 70% faster) |
| **F** | Toggle Flashlight / Torch |
| **H** | Toggle A* Breadcrumb Solution Trail |
| **F5** | Quick Save Game to Slot #1 |
| **F9** | Quick Load Game from Slot #1 |
| **ESC / P** | Pause / Resume Game |

---

## Project Structure
```text
MazeRunner_proj/
├── assets/
│   ├── shaders/        # GLSL 330 Core vertex & fragment shaders
│   └── textures/       # Materials & texture assets
├── bin/
│   └── mazerunner.exe  # Standalone production executable
├── data/
│   └── mazerunner.db   # Embedded SQLite database (schema, saves, leaderboard)
├── src/
│   ├── database.h/cpp  # SQLite persistence & save slot management
│   ├── game.h/cpp      # High-level game loop, objective checks, modes
│   ├── graphics.h/cpp  # OpenGL 3.3 Core 3D renderer & 2D HUD
│   ├── main.cpp        # Application entry point & CLI modes
│   ├── maze.h/cpp      # DFS, Prim, Wilson maze generation & 3D block conversion
│   ├── monster.h/cpp   # Autonomous FSM enemy AI (Patrol, Detect, Chase)
│   ├── pathfinding.h/cpp # BFS, A*, Bidirectional BFS pathfinding solvers
│   ├── player.h/cpp    # First-person camera, AABB collision sliding, stamina
│   └── tests.h/cpp     # 19 automated verification tests across Milestones 1-5
├── CMakeLists.txt      # Multiplatform CMake configuration
├── README.md           # Project documentation
└── Milestone_5_Final_Deployment_and_Evaluation_Report.pdf # Formal 3-page submission report
```
