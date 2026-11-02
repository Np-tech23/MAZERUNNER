#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "maze.h"
#include "pathfinding.h"

enum class MonsterState {
    PATROL,
    DETECT,
    CHASE,
    SEARCH
};

class Monster {
public:
    int id;
    glm::vec3 position;
    glm::vec3 forward;
    MonsterState state;
    float speed;
    float attack_damage;
    float attack_range;
    
    // AI Timers & targets
    glm::vec3 target_pos;
    glm::vec3 last_known_player_pos;
    float search_timer;
    float repath_timer;
    std::vector<glm::vec3> current_waypoints;
    size_t current_waypoint_idx;

    Monster(int id, glm::vec3 start_pos);

    void update(float dt, const glm::vec3& player_pos, const std::vector<std::string>& block_grid, const Maze& logical_maze);
    bool hasLineOfSight(const glm::vec3& target, const std::vector<std::string>& block_grid) const;
    void planPatrolPath(const std::vector<std::string>& block_grid);
    void planChasePath(const glm::vec3& target, const std::vector<std::string>& block_grid);

    std::string getStateName() const;
};

class MonsterManager {
public:
    std::vector<Monster> monsters;

    void spawnMonsters(int count, const std::vector<std::string>& block_grid, const std::vector<int>& spawn_candidates, int maze_w);
    void update(float dt, class Player& player, const std::vector<std::string>& block_grid, const Maze& logical_maze);
};
