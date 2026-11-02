#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

constexpr int KEY_BRONZE = (1 << 0);
constexpr int KEY_SILVER = (1 << 1);
constexpr int KEY_GOLD   = (1 << 2);
constexpr int ALL_KEYS_MASK = (KEY_BRONZE | KEY_SILVER | KEY_GOLD);

class Player {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 world_up;

    float yaw;
    float pitch;
    float move_speed;
    float sprint_multiplier;
    float mouse_sensitivity;

    // Attributes
    int health;
    int max_health;
    float stamina;
    float max_stamina;
    bool is_sprinting;
    bool torch_on;
    int keys_mask;
    float damage_invulnerability_timer;
    bool show_path_hint;

    Player(glm::vec3 start_pos = glm::vec3(1.5f, 0.5f, 1.5f));

    glm::mat4 getViewMatrix() const;
    void processKeyboard(char direction, float dt, const std::vector<std::string>& block_grid);
    void processMouseMovement(float xoffset, float yoffset, bool constrain_pitch = true);
    void update(float dt);

    void takeDamage(int amount);
    void heal(int amount);
    bool hasKey(int key_flag) const { return (keys_mask & key_flag) != 0; }
    void addKey(int key_flag) { keys_mask |= key_flag; }
    bool hasAllKeys() const { return (keys_mask & ALL_KEYS_MASK) == ALL_KEYS_MASK; }

private:
    void updateCameraVectors();
    bool checkCollision(const glm::vec3& test_pos, const std::vector<std::string>& block_grid, float radius = 0.25f) const;
};
