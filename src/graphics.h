#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

struct KeyItem {
    int id;
    int key_flag; // KEY_BRONZE, KEY_SILVER, KEY_GOLD
    glm::vec3 position;
    bool collected;
    glm::vec3 color;
    std::string name;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init(int width = 1280, int height = 720, const char* title = "MazeRunner 3D — Milestone 5");
    void cleanup();

    GLFWwindow* getWindow() const { return window_; }
    bool shouldClose() const { return glfwWindowShouldClose(window_); }
    void beginFrame();
    void endFrame();

    void setupMazeGeometry(const std::vector<std::string>& block_grid);
    void renderScene(const class Player& player,
                     const class MonsterManager& monster_mgr,
                     const std::vector<KeyItem>& keys,
                     const glm::vec3& exit_pos,
                     bool exit_unlocked,
                     const std::vector<int>& hint_path,
                     int maze_w, int maze_h,
                     float delta_time);

    void renderHUD(const class Player& player,
                   const class MonsterManager& monster_mgr,
                   const std::vector<KeyItem>& keys,
                   const std::vector<std::string>& block_grid,
                   float game_time,
                   float fps,
                   const std::string& status_msg,
                   bool is_time_trial,
                   float time_remaining);

    int getScreenWidth() const { return screen_width_; }
    int getScreenHeight() const { return screen_height_; }

private:
    GLFWwindow* window_ = nullptr;
    int screen_width_ = 1280;
    int screen_height_ = 720;

    // Shader programs
    GLuint world_shader_ = 0;
    GLuint hud_shader_ = 0;

    // 3D Geometry buffers
    GLuint cube_vao_ = 0;
    GLuint cube_vbo_ = 0;
    GLuint quad_vao_ = 0;
    GLuint quad_vbo_ = 0;

    // Maze Wall instances
    std::vector<glm::mat4> wall_transforms_;
    size_t wall_count_ = 0;
    int grid_w_ = 0;
    int grid_h_ = 0;

    float animation_time_ = 0.0f;

    void compileShaders();
    void createMeshes();
    void drawCube(const glm::mat4& model, const glm::vec3& color, float roughness = 0.5f);
    void drawQuad2D(float x, float y, float w, float h, const glm::vec4& color);
};
