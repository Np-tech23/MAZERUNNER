#include "player.h"
#include <cmath>
#include <algorithm>

Player::Player(glm::vec3 start_pos)
    : position(start_pos),
      world_up(0.0f, 1.0f, 0.0f),
      yaw(-90.0f),
      pitch(0.0f),
      move_speed(4.0f),
      sprint_multiplier(1.7f),
      mouse_sensitivity(0.12f),
      health(100),
      max_health(100),
      stamina(100.0f),
      max_stamina(100.0f),
      is_sprinting(false),
      torch_on(true),
      keys_mask(0),
      damage_invulnerability_timer(3.0f),
      show_path_hint(false)
{
    updateCameraVectors();
}

glm::mat4 Player::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

void Player::updateCameraVectors() {
    glm::vec3 f;
    f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    f.y = sin(glm::radians(pitch));
    f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(f);
    right = glm::normalize(glm::cross(front, world_up));
    up    = glm::normalize(glm::cross(right, front));
}

void Player::processMouseMovement(float xoffset, float yoffset, bool constrain_pitch) {
    xoffset *= mouse_sensitivity;
    yoffset *= mouse_sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if (constrain_pitch) {
        if (pitch > 89.0f)  pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    updateCameraVectors();
}

bool Player::checkCollision(const glm::vec3& test_pos, const std::vector<std::string>& block_grid, float radius) const {
    if (block_grid.empty()) return false;
    int h = static_cast<int>(block_grid.size());
    int w = static_cast<int>(block_grid[0].size());

    // Check corners around the circle/AABB
    int min_x = std::max(0, static_cast<int>(std::floor(test_pos.x - radius)));
    int max_x = std::min(w - 1, static_cast<int>(std::floor(test_pos.x + radius)));
    int min_z = std::max(0, static_cast<int>(std::floor(test_pos.z - radius)));
    int max_z = std::min(h - 1, static_cast<int>(std::floor(test_pos.z + radius)));

    for (int gz = min_z; gz <= max_z; ++gz) {
        for (int gx = min_x; gx <= max_x; ++gx) {
            if (block_grid[gz][gx] == '#') {
                return true; // Wall collision
            }
        }
    }
    return false;
}

void Player::processKeyboard(char direction, float dt, const std::vector<std::string>& block_grid) {
    float effective_speed = move_speed;
    if (is_sprinting && stamina > 5.0f) {
        effective_speed *= sprint_multiplier;
        stamina -= 25.0f * dt;
        if (stamina < 0.0f) stamina = 0.0f;
    } else {
        is_sprinting = false;
    }

    float velocity = effective_speed * dt;
    glm::vec3 walk_front = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    glm::vec3 walk_right = glm::normalize(glm::vec3(right.x, 0.0f, right.z));

    glm::vec3 delta(0.0f);
    if (direction == 'W') delta += walk_front * velocity;
    if (direction == 'S') delta -= walk_front * velocity;
    if (direction == 'A') delta -= walk_right * velocity;
    if (direction == 'D') delta += walk_right * velocity;

    // Sliding collision resolution:
    // First try full move
    glm::vec3 target_pos = position + delta;
    if (!checkCollision(target_pos, block_grid)) {
        position = target_pos;
    } else {
        // Try sliding along X only
        glm::vec3 target_x = position + glm::vec3(delta.x, 0.0f, 0.0f);
        if (!checkCollision(target_x, block_grid)) {
            position = target_x;
        }
        // Try sliding along Z only
        glm::vec3 target_z = position + glm::vec3(0.0f, 0.0f, delta.z);
        if (!checkCollision(target_z, block_grid)) {
            position = target_z;
        }
    }
}

void Player::update(float dt) {
    // Regenerate stamina when not sprinting
    if (!is_sprinting && stamina < max_stamina) {
        stamina += 15.0f * dt;
        if (stamina > max_stamina) stamina = max_stamina;
    }

    // Cooldown invulnerability
    if (damage_invulnerability_timer > 0.0f) {
        damage_invulnerability_timer -= dt;
        if (damage_invulnerability_timer < 0.0f) damage_invulnerability_timer = 0.0f;
    }
}

void Player::takeDamage(int amount) {
    if (damage_invulnerability_timer <= 0.0f) {
        health -= amount;
        if (health < 0) health = 0;
        damage_invulnerability_timer = 1.0f; // 1 second invulnerability
    }
}

void Player::heal(int amount) {
    health += amount;
    if (health > max_health) health = max_health;
}
