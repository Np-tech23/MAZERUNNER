#include "game.h"
#include <iostream>
#include <algorithm>
#include <chrono>

static Game* g_active_game = nullptr;

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window;
    if (g_active_game) {
        g_active_game->onKey(key, scancode, action, mods);
    }
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    (void)window;
    if (g_active_game) {
        g_active_game->onMouseMove(xpos, ypos);
    }
}

Game::Game() : maze_(nullptr) {
    g_active_game = this;
    for (int i = 0; i < 1024; ++i) keys_down_[i] = false;
}

Game::~Game() {
    if (maze_) delete maze_;
    if (g_active_game == this) g_active_game = nullptr;
}

void Game::spawnKeysAndExit() {
    keys_.clear();
    auto dead_ends = maze_->getDeadEnds();

    // Ensure we have candidates for 3 keys
    std::vector<int> key_cells;
    int goal_cell = maze_->width * maze_->height - 1;

    for (int de : dead_ends) {
        if (de != 0 && de != goal_cell) {
            key_cells.push_back(de);
        }
    }

    // If not enough dead-ends, pick distant regular cells
    if (key_cells.size() < 3) {
        for (int i = 1; i < maze_->width * maze_->height - 1; ++i) {
            if (std::find(key_cells.begin(), key_cells.end(), i) == key_cells.end()) {
                key_cells.push_back(i);
            }
            if (key_cells.size() >= 3) break;
        }
    }

    // Key 1: Bronze
    if (key_cells.size() > 0) {
        int c = key_cells[0];
        int gx = 2 * (c % maze_->width) + 1;
        int gz = 2 * (c / maze_->width) + 1;
        keys_.push_back({1, KEY_BRONZE, glm::vec3(gx + 0.5f, 0.5f, gz + 0.5f), false, glm::vec3(0.85f, 0.53f, 0.25f), "Bronze Key"});
    }
    // Key 2: Silver
    if (key_cells.size() > 1) {
        int c = key_cells[1];
        int gx = 2 * (c % maze_->width) + 1;
        int gz = 2 * (c / maze_->width) + 1;
        keys_.push_back({2, KEY_SILVER, glm::vec3(gx + 0.5f, 0.5f, gz + 0.5f), false, glm::vec3(0.85f, 0.88f, 0.95f), "Silver Key"});
    }
    // Key 3: Gold
    if (key_cells.size() > 2) {
        int c = key_cells[2];
        int gx = 2 * (c % maze_->width) + 1;
        int gz = 2 * (c / maze_->width) + 1;
        keys_.push_back({3, KEY_GOLD, glm::vec3(gx + 0.5f, 0.5f, gz + 0.5f), false, glm::vec3(1.0f, 0.85f, 0.15f), "Gold Key"});
    }

    // Exit Gate position (opposite corner)
    int exit_cx = maze_->width - 1;
    int exit_cy = maze_->height - 1;
    exit_pos_ = glm::vec3(2 * exit_cx + 1 + 0.5f, 0.9f, 2 * exit_cy + 1 + 0.5f);
    exit_unlocked_ = false;

    // Monsters spawn in the opposite quadrant / far from player start (0,0)
    std::vector<int> monster_spawns;
    for (int y = maze_->height / 2; y < maze_->height; ++y) {
        for (int x = maze_->width / 2; x < maze_->width; ++x) {
            int idx = y * maze_->width + x;
            if (idx != (maze_->height - 1) * maze_->width + (maze_->width - 1)) {
                monster_spawns.push_back(idx);
            }
        }
    }
    if (monster_spawns.empty()) {
        monster_spawns.push_back(maze_->width * maze_->height - 2);
    }
    int monster_count = (mode_ == "SURVIVAL") ? 5 : ((mode_ == "TIME_TRIAL") ? 3 : 2);
    monster_mgr_.spawnMonsters(monster_count, block_grid_, monster_spawns, maze_->width);
}

void Game::updateHintPath() {
    if (!maze_) return;
    int start_node = 0;
    int cur_cx = static_cast<int>(std::floor(player_.position.x)) / 2;
    int cur_cy = static_cast<int>(std::floor(player_.position.z)) / 2;
    cur_cx = std::clamp(cur_cx, 0, maze_->width - 1);
    cur_cy = std::clamp(cur_cy, 0, maze_->height - 1);
    start_node = cur_cy * maze_->width + cur_cx;

    // Target is next uncollected key or exit portal
    int goal_node = (maze_->height - 1) * maze_->width + (maze_->width - 1);
    for (const auto& k : keys_) {
        if (!k.collected) {
            int kx = static_cast<int>(std::floor(k.position.x)) / 2;
            int kz = static_cast<int>(std::floor(k.position.z)) / 2;
            goal_node = kz * maze_->width + kx;
            break;
        }
    }

    PathfindingStats stats = astar(*maze_, start_node, goal_node);
    hint_path_ = stats.path;
}

bool Game::init(int maze_w, int maze_h, int seed, const std::string& mode) {
    maze_w_ = maze_w;
    maze_h_ = maze_h;
    seed_ = seed;
    mode_ = mode;
    state_ = GameState::PLAYING;
    game_time_ = 0.0f;
    score_ = 0;
    time_limit_ = (mode_ == "TIME_TRIAL") ? 300.0f : 999999.0f;

    db_.open("data/mazerunner.db");

    if (maze_) delete maze_;
    maze_ = new Maze(maze_w_, maze_h_);
    generate_DFS(*maze_, seed_);

    block_grid_ = maze_->toBlockGrid();
    player_ = Player(glm::vec3(1.5f, 0.5f, 1.5f)); // Entrance is (1, 1) in block grid

    spawnKeysAndExit();
    updateHintPath();

    if (!renderer_.init(1280, 720, "MazeRunner 3D — Milestone 5")) {
        std::cerr << "[Game] Failed to initialize Renderer window.\n";
        return false;
    }

    renderer_.setupMazeGeometry(block_grid_);

    // Register GLFW callbacks
    GLFWwindow* win = renderer_.getWindow();
    glfwSetKeyCallback(win, keyCallback);
    glfwSetCursorPosCallback(win, cursorPosCallback);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    std::cout << "[Game] Initialized successfully: Mode=" << mode_ << " Size=" << maze_w_ << "x" << maze_h_ << "\n";
    return true;
}

void Game::onKey(int key, int scancode, int action, int mods) {
    (void)scancode; (void)mods;
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) keys_down_[key] = true;
        else if (action == GLFW_RELEASE) keys_down_[key] = false;
    }

    if (action == GLFW_PRESS) {
        if (state_ == GameState::GAME_OVER || state_ == GameState::VICTORY) {
            if (key == GLFW_KEY_R) {
                resetLevel();
                return;
            } else if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_ENTER) {
                glfwSetWindowShouldClose(renderer_.getWindow(), GLFW_TRUE);
                return;
            }
        }

        if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_P) {
            if (state_ == GameState::PLAYING) {
                state_ = GameState::PAUSED;
                glfwSetInputMode(renderer_.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else if (state_ == GameState::PAUSED) {
                state_ = GameState::PLAYING;
                glfwSetInputMode(renderer_.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                first_mouse_ = true;
            }
        } else if (key == GLFW_KEY_F) {
            player_.torch_on = !player_.torch_on;
        } else if (key == GLFW_KEY_H) {
            player_.show_path_hint = !player_.show_path_hint;
            if (player_.show_path_hint) updateHintPath();
        } else if (key == GLFW_KEY_F5) {
            save(1);
        } else if (key == GLFW_KEY_F9) {
            load(1);
        }
    }
}

void Game::onMouseMove(double xpos, double ypos) {
    if (state_ != GameState::PLAYING) return;

    if (first_mouse_) {
        last_x_ = static_cast<float>(xpos);
        last_y_ = static_cast<float>(ypos);
        first_mouse_ = false;
    }

    float xoffset = static_cast<float>(xpos) - last_x_;
    float yoffset = last_y_ - static_cast<float>(ypos); // Reversed since y-coordinates range from bottom to top
    last_x_ = static_cast<float>(xpos);
    last_y_ = static_cast<float>(ypos);

    player_.processMouseMovement(xoffset, yoffset);
}

void Game::checkInteractions() {
    // 1. Key collection
    for (auto& k : keys_) {
        if (!k.collected) {
            float dist = glm::distance(glm::vec3(player_.position.x, 0.0f, player_.position.z),
                                       glm::vec3(k.position.x, 0.0f, k.position.z));
            if (dist < 0.9f) {
                k.collected = true;
                player_.addKey(k.key_flag);
                score_ += 500;
                status_msg_ = "Collected " + k.name + "! (" + std::to_string(player_.keys_mask) + "/7)";
                std::cout << "[Game] " << status_msg_ << "\n";
                updateHintPath();
            }
        }
    }

    // 2. Exit door unlock check
    if (player_.hasAllKeys() && !exit_unlocked_) {
        exit_unlocked_ = true;
        score_ += 1000;
        status_msg_ = "ALL 3 KEYS ACQUIRED! EXIT PORTAL UNLOCKED!";
        std::cout << "[Game] " << status_msg_ << "\n";
        updateHintPath();
    }

    // 3. Victory check
    float dist_to_exit = glm::distance(glm::vec3(player_.position.x, 0.0f, player_.position.z),
                                      glm::vec3(exit_pos_.x, 0.0f, exit_pos_.z));
    if (exit_unlocked_ && dist_to_exit < 1.0f) {
        state_ = GameState::VICTORY;
        int time_bonus = std::max(0, static_cast<int>((time_limit_ - game_time_) * 10.0f));
        score_ += time_bonus + 2000;
        status_msg_ = "VICTORY! You escaped the maze in " + std::to_string(static_cast<int>(game_time_)) + " seconds!";
        std::cout << "\n==================== VICTORY! ====================\n"
                  << status_msg_ << "\nFinal Score: " << score_ << "\n"
                  << "==================================================\n";
        
        SessionRecord s;
        s.player_name = "Niyati Patel";
        s.mode = mode_;
        s.outcome = "ESCAPED";
        s.duration_ms = static_cast<int>(game_time_ * 1000.0f);
        s.score = score_;
        s.keys_collected = 3;
        s.monsters_evaded = monster_mgr_.monsters.size();
        db_.recordSession(s);
        glfwSetInputMode(renderer_.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // 4. Game over check
    if (player_.health <= 0) {
        state_ = GameState::GAME_OVER;
        status_msg_ = "GAME OVER: Slain by the labyrinth monster.";
        std::cout << "\n==================== GAME OVER ====================\n"
                  << status_msg_ << "\nFinal Score: " << score_ << "\n"
                  << "===================================================\n";

        SessionRecord s;
        s.player_name = "Niyati Patel";
        s.mode = mode_;
        s.outcome = "DIED";
        s.duration_ms = static_cast<int>(game_time_ * 1000.0f);
        s.score = score_;
        s.keys_collected = (player_.hasKey(KEY_BRONZE)?1:0) + (player_.hasKey(KEY_SILVER)?1:0) + (player_.hasKey(KEY_GOLD)?1:0);
        s.monsters_evaded = 0;
        db_.recordSession(s);
        glfwSetInputMode(renderer_.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // 5. Time trial timeout
    if (mode_ == "TIME_TRIAL" && game_time_ >= time_limit_) {
        state_ = GameState::GAME_OVER;
        status_msg_ = "GAME OVER: Time expired!";
        glfwSetInputMode(renderer_.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void Game::update(float dt) {
    if (state_ != GameState::PLAYING) return;

    game_time_ += dt;

    // Keyboard input processing
    player_.is_sprinting = keys_down_[GLFW_KEY_LEFT_SHIFT];
    if (keys_down_[GLFW_KEY_W]) player_.processKeyboard('W', dt, block_grid_);
    if (keys_down_[GLFW_KEY_S]) player_.processKeyboard('S', dt, block_grid_);
    if (keys_down_[GLFW_KEY_A]) player_.processKeyboard('A', dt, block_grid_);
    if (keys_down_[GLFW_KEY_D]) player_.processKeyboard('D', dt, block_grid_);

    player_.update(dt);
    monster_mgr_.update(dt, player_, block_grid_, *maze_);
    checkInteractions();
}

void Game::render() {
    renderer_.beginFrame();

    renderer_.renderScene(player_, monster_mgr_, keys_, exit_pos_, exit_unlocked_,
                         hint_path_, maze_w_, maze_h_, 0.016f);

    float time_rem = std::max(0.0f, time_limit_ - game_time_);
    renderer_.renderHUD(player_, monster_mgr_, keys_, block_grid_, game_time_,
                       144.0f, status_msg_, (mode_ == "TIME_TRIAL"), time_rem);

    renderer_.endFrame();
}

bool Game::save(int slot_id) {
    SaveSlotData d;
    d.slot_id = slot_id;
    d.player_name = "Niyati Patel";
    d.maze_w = maze_w_;
    d.maze_h = maze_h_;
    d.seed = seed_;
    d.player_x = player_.position.x;
    d.player_y = player_.position.y;
    d.player_z = player_.position.z;
    d.yaw = player_.yaw;
    d.pitch = player_.pitch;
    d.health = player_.health;
    d.stamina = player_.stamina;
    d.keys_mask = player_.keys_mask;
    d.time_elapsed = game_time_;
    d.score = score_;

    bool ok = db_.saveSlot(slot_id, d);
    if (ok) status_msg_ = "Game successfully saved to Slot #" + std::to_string(slot_id);
    return ok;
}

bool Game::load(int slot_id) {
    SaveSlotData d;
    if (!db_.loadSlot(slot_id, d) || d.is_empty) {
        status_msg_ = "Slot #" + std::to_string(slot_id) + " is empty!";
        return false;
    }

    player_.position = glm::vec3(d.player_x, d.player_y, d.player_z);
    player_.yaw = d.yaw;
    player_.pitch = d.pitch;
    player_.health = d.health;
    player_.stamina = d.stamina;
    player_.keys_mask = d.keys_mask;
    game_time_ = d.time_elapsed;
    score_ = d.score;

    // Refresh key collected states
    for (auto& k : keys_) {
        if (player_.hasKey(k.key_flag)) k.collected = true;
    }
    if (player_.hasAllKeys()) exit_unlocked_ = true;

    status_msg_ = "Game loaded from Slot #" + std::to_string(slot_id);
    updateHintPath();
    return true;
}

void Game::resetLevel(int seed) {
    if (seed == 0) seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count() % 100000);
    seed_ = seed;
    state_ = GameState::PLAYING;
    game_time_ = 0.0f;
    score_ = 0;
    exit_unlocked_ = false;

    if (maze_) delete maze_;
    maze_ = new Maze(maze_w_, maze_h_);
    generate_DFS(*maze_, seed_);
    block_grid_ = maze_->toBlockGrid();

    player_ = Player(glm::vec3(1.5f, 0.5f, 1.5f));
    spawnKeysAndExit();
    updateHintPath();

    renderer_.setupMazeGeometry(block_grid_);
    glfwSetInputMode(renderer_.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    first_mouse_ = true;
    status_msg_ = "Find all 3 keys to unlock the Exit Gate! [ESC=Pause, H=Hint]";
}

void Game::run() {
    auto last_time = std::chrono::high_resolution_clock::now();

    while (!renderer_.shouldClose()) {
        auto current_time = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(current_time - last_time).count();
        last_time = current_time;

        if (dt > 0.1f) dt = 0.1f;

        update(dt);
        render();
    }
}

