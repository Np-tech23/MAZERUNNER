#include "monster.h"
#include "player.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>

Monster::Monster(int id, glm::vec3 start_pos)
    : id(id),
      position(start_pos),
      forward(0.0f, 0.0f, 1.0f),
      state(MonsterState::PATROL),
      speed(2.0f),
      attack_damage(25.0f),
      attack_range(0.9f),
      target_pos(start_pos),
      last_known_player_pos(start_pos),
      search_timer(0.0f),
      repath_timer(0.0f),
      current_waypoint_idx(0)
{
}

std::string Monster::getStateName() const {
    switch (state) {
        case MonsterState::PATROL: return "PATROL";
        case MonsterState::DETECT: return "DETECT";
        case MonsterState::CHASE:  return "CHASE";
        case MonsterState::SEARCH: return "SEARCH";
    }
    return "UNKNOWN";
}

bool Monster::hasLineOfSight(const glm::vec3& target, const std::vector<std::string>& block_grid) const {
    if (block_grid.empty()) return false;
    int h = static_cast<int>(block_grid.size());
    int w = static_cast<int>(block_grid[0].size());

    // Raycast from monster position to target position
    float dx = target.x - position.x;
    float dz = target.z - position.z;
    float dist = std::sqrt(dx * dx + dz * dz);
    if (dist > 12.0f) return false; // Beyond sight range

    int steps = static_cast<int>(dist * 4.0f) + 1;
    for (int i = 1; i < steps; ++i) {
        float t = static_cast<float>(i) / steps;
        int check_x = static_cast<int>(std::floor(position.x + dx * t));
        int check_z = static_cast<int>(std::floor(position.z + dz * t));

        if (check_x < 0 || check_x >= w || check_z < 0 || check_z >= h) return false;
        if (block_grid[check_z][check_x] == '#') return false; // Blocked by wall
    }
    return true;
}

void Monster::planPatrolPath(const std::vector<std::string>& block_grid) {
    if (block_grid.empty()) return;
    int h = static_cast<int>(block_grid.size());
    int w = static_cast<int>(block_grid[0].size());

    int cur_gx = static_cast<int>(std::floor(position.x));
    int cur_gz = static_cast<int>(std::floor(position.z));

    // Pick a random open cell within radius ~8
    std::vector<glm::vec3> candidates;
    for (int rz = -6; rz <= 6; ++rz) {
        for (int rx = -6; rx <= 6; ++rx) {
            int gx = cur_gx + rx;
            int gz = cur_gz + rz;
            if (gx > 0 && gx < w - 1 && gz > 0 && gz < h - 1 && block_grid[gz][gx] != '#') {
                candidates.push_back(glm::vec3(gx + 0.5f, 0.5f, gz + 0.5f));
            }
        }
    }

    current_waypoints.clear();
    current_waypoint_idx = 0;

    if (!candidates.empty()) {
        glm::vec3 chosen = candidates[std::rand() % candidates.size()];
        current_waypoints.push_back(chosen);
    }
}

void Monster::planChasePath(const glm::vec3& target, const std::vector<std::string>& block_grid) {
    current_waypoints.clear();
    current_waypoint_idx = 0;

    int cur_x = static_cast<int>(std::floor(position.x));
    int cur_z = static_cast<int>(std::floor(position.z));
    int tgt_x = static_cast<int>(std::floor(target.x));
    int tgt_z = static_cast<int>(std::floor(target.z));

    int h = static_cast<int>(block_grid.size());
    int w = static_cast<int>(block_grid[0].size());

    if (cur_x == tgt_x && cur_z == tgt_z) {
        current_waypoints.push_back(target);
        return;
    }

    // Simple grid BFS to build waypoints in block grid
    std::vector<int> parent(w * h, -1);
    std::vector<bool> visited(w * h, false);
    std::vector<int> q;
    int start_node = cur_z * w + cur_x;
    int goal_node = tgt_z * w + tgt_x;

    q.push_back(start_node);
    visited[start_node] = true;
    size_t head = 0;

    int dx[4] = {0, 1, 0, -1};
    int dz[4] = {-1, 0, 1, 0};

    bool found = false;
    while (head < q.size()) {
        int u = q[head++];
        if (u == goal_node) {
            found = true;
            break;
        }
        int ux = u % w;
        int uz = u / w;
        for (int d = 0; d < 4; ++d) {
            int nx = ux + dx[d];
            int nz = uz + dz[d];
            if (nx >= 0 && nx < w && nz >= 0 && nz < h && block_grid[nz][nx] != '#') {
                int v = nz * w + nx;
                if (!visited[v]) {
                    visited[v] = true;
                    parent[v] = u;
                    q.push_back(v);
                }
            }
        }
    }

    if (found) {
        std::vector<glm::vec3> path;
        int curr = goal_node;
        while (curr != start_node) {
            int cx = curr % w;
            int cz = curr / w;
            path.push_back(glm::vec3(cx + 0.5f, 0.5f, cz + 0.5f));
            curr = parent[curr];
        }
        std::reverse(path.begin(), path.end());
        current_waypoints = path;
    } else {
        current_waypoints.push_back(target);
    }
}

void Monster::update(float dt, const glm::vec3& player_pos, const std::vector<std::string>& block_grid, const Maze& logical_maze) {
    (void)logical_maze;
    float dist_to_player = glm::distance(glm::vec3(position.x, 0.0f, position.z), glm::vec3(player_pos.x, 0.0f, player_pos.z));
    bool see_player = hasLineOfSight(player_pos, block_grid);

    repath_timer -= dt;

    // FSM State Transitions
    switch (state) {
        case MonsterState::PATROL:
            speed = 2.0f;
            if (see_player && dist_to_player <= 10.0f) {
                state = MonsterState::DETECT;
                search_timer = 0.3f; // Brief alert reaction
            }
            break;

        case MonsterState::DETECT:
            search_timer -= dt;
            if (search_timer <= 0.0f) {
                state = MonsterState::CHASE;
                last_known_player_pos = player_pos;
                planChasePath(player_pos, block_grid);
                repath_timer = 0.5f;
            }
            break;

        case MonsterState::CHASE:
            speed = 4.5f;
            if (see_player) {
                last_known_player_pos = player_pos;
                if (repath_timer <= 0.0f) {
                    planChasePath(player_pos, block_grid);
                    repath_timer = 0.4f;
                }
            } else {
                // Lost sight -> switch to search at last known pos
                state = MonsterState::SEARCH;
                search_timer = 2.5f;
                planChasePath(last_known_player_pos, block_grid);
            }
            break;

        case MonsterState::SEARCH:
            speed = 3.0f;
            if (see_player) {
                state = MonsterState::CHASE;
                last_known_player_pos = player_pos;
                planChasePath(player_pos, block_grid);
                repath_timer = 0.4f;
            } else {
                search_timer -= dt;
                if (search_timer <= 0.0f) {
                    state = MonsterState::PATROL;
                    planPatrolPath(block_grid);
                }
            }
            break;
    }

    // Movement along current waypoints
    if (current_waypoint_idx < current_waypoints.size()) {
        glm::vec3 target = current_waypoints[current_waypoint_idx];
        glm::vec3 dir = target - position;
        dir.y = 0.0f;
        float d = glm::length(dir);
        if (d > 0.1f) {
            dir = glm::normalize(dir);
            forward = dir;
            position += dir * speed * dt;
        } else {
            current_waypoint_idx++;
        }
    } else {
        if (state == MonsterState::PATROL) {
            planPatrolPath(block_grid);
        }
    }
}

void MonsterManager::spawnMonsters(int count, const std::vector<std::string>& block_grid, const std::vector<int>& spawn_candidates, int maze_w) {
    monsters.clear();
    int id_counter = 1;

    for (int candidate : spawn_candidates) {
        if (static_cast<int>(monsters.size()) >= count) break;
        int cx = candidate % maze_w;
        int cy = candidate / maze_w;
        // Transform logical maze cell to block grid coordinates: (2*cx+1, 2*cy+1)
        int gx = 2 * cx + 1;
        int gz = 2 * cy + 1;

        if (gz < static_cast<int>(block_grid.size()) && gx < static_cast<int>(block_grid[0].size())) {
            if (block_grid[gz][gx] != '#') {
                glm::vec3 spawn_pos(gx + 0.5f, 0.5f, gz + 0.5f);
                monsters.emplace_back(id_counter++, spawn_pos);
            }
        }
    }

    std::cout << "[MonsterManager] Spawned " << monsters.size() << " autonomous AI monsters into the maze.\n";
}

void MonsterManager::update(float dt, Player& player, const std::vector<std::string>& block_grid, const Maze& logical_maze) {
    for (auto& m : monsters) {
        m.update(dt, player.position, block_grid, logical_maze);

        // Proximity attack check
        float dist = glm::distance(glm::vec3(m.position.x, 0.0f, m.position.z), glm::vec3(player.position.x, 0.0f, player.position.z));
        if (dist <= m.attack_range) {
            player.takeDamage(static_cast<int>(m.attack_damage));
        }
    }
}
