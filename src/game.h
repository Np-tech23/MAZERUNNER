#pragma once
#include <string>
#include <vector>
#include "maze.h"
#include "pathfinding.h"
#include "player.h"
#include "monster.h"
#include "database.h"
#include "graphics.h"

enum class GameState {
    PLAYING,
    PAUSED,
    GAME_OVER,
    VICTORY
};

class Game {
public:
    Game();
    ~Game();

    bool init(int maze_w = 15, int maze_h = 10, int seed = 42, const std::string& mode = "STANDARD");
    void run();

    void update(float dt);
    void render();

    bool save(int slot_id);
    bool load(int slot_id);

    void resetLevel(int seed = 0);

    // Callbacks
    void onKey(int key, int scancode, int action, int mods);
    void onMouseMove(double xpos, double ypos);

    GameState getState() const { return state_; }
    void setState(GameState s) { state_ = s; }

private:
    int maze_w_ = 15;
    int maze_h_ = 10;
    int seed_ = 42;
    std::string mode_ = "STANDARD";
    GameState state_ = GameState::PLAYING;

    Maze* maze_ = nullptr;
    std::vector<std::string> block_grid_;
    Player player_;
    MonsterManager monster_mgr_;
    Renderer renderer_;
    Database db_;

    std::vector<KeyItem> keys_;
    glm::vec3 exit_pos_;
    bool exit_unlocked_ = false;

    // Hint path from A*
    std::vector<int> hint_path_;

    // Timers & scores
    float game_time_ = 0.0f;
    float time_limit_ = 300.0f; // 5 min for Time Trial
    int score_ = 0;
    int monsters_evaded_ = 0;
    std::string status_msg_ = "Find all 3 keys to unlock the Exit Gate!";

    // Input state
    bool first_mouse_ = true;
    float last_x_ = 640.0f;
    float last_y_ = 360.0f;
    bool keys_down_[1024] = {false};

    void spawnKeysAndExit();
    void updateHintPath();
    void checkInteractions();
};
