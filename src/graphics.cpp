#include "graphics.h"
#include "player.h"
#include "monster.h"
#include <iostream>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

// Vertex data for standard 1x1x1 cube (centered at 0,0,0)
static const float CUBE_VERTICES[] = {
    // Positions          // Normals
    // Back face
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    // Front face
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

    // Left face
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

    // Right face
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    // Bottom face
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    // Top face
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f
};

// Quad vertices for 2D HUD
static const float QUAD_VERTICES[] = {
    // Pos (X, Y)
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,

    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f
};

// Shaders
static const char* WORLD_VS = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

static const char* WORLD_FS = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 objectColor;
uniform vec3 viewPos;
uniform vec3 torchDir;
uniform bool torchOn;

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Baseline ambient light
    vec3 ambient = 0.22 * objectColor;

    // Torchlight / Flashlight (Spotlight)
    vec3 lightDir = normalize(viewPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * objectColor;

    // Specular
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = 0.3 * spec * vec3(1.0);

    float distance = length(viewPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));

    float theta = dot(lightDir, normalize(-torchDir));
    float cutOff = cos(radians(14.0));
    float outerCutOff = cos(radians(22.0));
    float epsilon = cutOff - outerCutOff;
    float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

    vec3 lighting = ambient;
    if (torchOn) {
        lighting += (diffuse + specular) * attenuation * intensity * 2.2;
    } else {
        lighting += diffuse * attenuation * 0.4;
    }

    // Distance Exponential Fog
    float fogDensity = 0.045;
    float fogFactor = exp(-pow(distance * fogDensity, 1.6));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 fogColor = vec3(0.04, 0.06, 0.09); // Deep dark atmosphere

    vec3 finalColor = mix(fogColor, lighting, fogFactor);
    FragColor = vec4(finalColor, 1.0);
}
)";

static const char* HUD_VS = R"(
#version 330 core
layout (location = 0) in vec2 aPos;

uniform mat4 projection;
uniform vec2 position;
uniform vec2 size;

void main() {
    vec2 pos = position + aPos * size;
    gl_Position = projection * vec4(pos, 0.0, 1.0);
}
)";

static const char* HUD_FS = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 color;

void main() {
    FragColor = color;
}
)";

Renderer::Renderer() {}

Renderer::~Renderer() {
    cleanup();
}

static GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "[Shader Error] Compilation failed:\n" << infoLog << "\n";
    }
    return shader;
}

void Renderer::compileShaders() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, WORLD_VS);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, WORLD_FS);
    world_shader_ = glCreateProgram();
    glAttachShader(world_shader_, vs);
    glAttachShader(world_shader_, fs);
    glLinkProgram(world_shader_);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLuint hvs = compileShader(GL_VERTEX_SHADER, HUD_VS);
    GLuint hfs = compileShader(GL_FRAGMENT_SHADER, HUD_FS);
    hud_shader_ = glCreateProgram();
    glAttachShader(hud_shader_, hvs);
    glAttachShader(hud_shader_, hfs);
    glLinkProgram(hud_shader_);
    glDeleteShader(hvs);
    glDeleteShader(hfs);
}

void Renderer::createMeshes() {
    // 3D Cube VAO / VBO
    glGenVertexArrays(1, &cube_vao_);
    glGenBuffers(1, &cube_vbo_);

    glBindVertexArray(cube_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTICES), CUBE_VERTICES, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 2D Quad VAO / VBO
    glGenVertexArrays(1, &quad_vao_);
    glGenBuffers(1, &quad_vbo_);

    glBindVertexArray(quad_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTICES), QUAD_VERTICES, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

bool Renderer::init(int width, int height, const char* title) {
    screen_width_ = width;
    screen_height_ = height;

    if (!glfwInit()) {
        std::cerr << "[Renderer] Failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window_ = glfwCreateWindow(screen_width_, screen_height_, title, nullptr, nullptr);
    if (!window_) {
        std::cerr << "[Renderer] Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable VSync

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "[Renderer] Failed to initialize GLEW\n";
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    compileShaders();
    createMeshes();

    std::cout << "[Renderer] OpenGL 3.3 Core Profile initialized successfully.\n";
    std::cout << "[Renderer] GPU: " << glGetString(GL_RENDERER) << " | Driver: " << glGetString(GL_VERSION) << "\n";
    return true;
}

void Renderer::cleanup() {
    if (cube_vao_) glDeleteVertexArrays(1, &cube_vao_);
    if (cube_vbo_) glDeleteBuffers(1, &cube_vbo_);
    if (quad_vao_) glDeleteVertexArrays(1, &quad_vao_);
    if (quad_vbo_) glDeleteBuffers(1, &quad_vbo_);
    if (world_shader_) glDeleteProgram(world_shader_);
    if (hud_shader_) glDeleteProgram(hud_shader_);

    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

void Renderer::beginFrame() {
    glfwPollEvents();
    glClearColor(0.04f, 0.06f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {
    glfwSwapBuffers(window_);
}

void Renderer::setupMazeGeometry(const std::vector<std::string>& block_grid) {
    wall_transforms_.clear();
    grid_h_ = static_cast<int>(block_grid.size());
    grid_w_ = block_grid.empty() ? 0 : static_cast<int>(block_grid[0].size());

    for (int z = 0; z < grid_h_; ++z) {
        for (int x = 0; x < grid_w_; ++x) {
            if (block_grid[z][x] == '#') {
                // Ground block and upper wall block (2 units high)
                glm::mat4 m1 = glm::translate(glm::mat4(1.0f), glm::vec3(x + 0.5f, 0.5f, z + 0.5f));
                glm::mat4 m2 = glm::translate(glm::mat4(1.0f), glm::vec3(x + 0.5f, 1.5f, z + 0.5f));
                wall_transforms_.push_back(m1);
                wall_transforms_.push_back(m2);
            }
        }
    }
    wall_count_ = wall_transforms_.size();
}

void Renderer::drawCube(const glm::mat4& model, const glm::vec3& color, float roughness) {
    (void)roughness;
    glUniformMatrix4fv(glGetUniformLocation(world_shader_, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(world_shader_, "objectColor"), 1, glm::value_ptr(color));
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Renderer::renderScene(const Player& player,
                           const MonsterManager& monster_mgr,
                           const std::vector<KeyItem>& keys,
                           const glm::vec3& exit_pos,
                           bool exit_unlocked,
                           const std::vector<int>& hint_path,
                           int maze_w, int maze_h,
                           float delta_time)
{
    (void)maze_h;
    animation_time_ += delta_time;

    glEnable(GL_DEPTH_TEST);
    glUseProgram(world_shader_);

    glm::mat4 view = player.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(75.0f), (float)screen_width_ / screen_height_, 0.1f, 100.0f);

    glUniformMatrix4fv(glGetUniformLocation(world_shader_, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(world_shader_, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(world_shader_, "viewPos"), 1, glm::value_ptr(player.position));
    glUniform3fv(glGetUniformLocation(world_shader_, "torchDir"), 1, glm::value_ptr(player.front));
    glUniform1i(glGetUniformLocation(world_shader_, "torchOn"), player.torch_on ? 1 : 0);

    glBindVertexArray(cube_vao_);

    // 1. Draw Maze Walls (Stone color)
    glm::vec3 stone_color(0.42f, 0.44f, 0.48f);
    for (const auto& model : wall_transforms_) {
        // Frustum / distance culling: only draw walls within distance ~22 units
        glm::vec3 wall_pos(model[3][0], model[3][1], model[3][2]);
        if (glm::distance(player.position, wall_pos) < 24.0f) {
            drawCube(model, stone_color);
        }
    }

    // 2. Draw Floor & Ceiling for each open cell near player
    int px = static_cast<int>(std::floor(player.position.x));
    int pz = static_cast<int>(std::floor(player.position.z));
    int render_radius = 18;

    for (int cz = std::max(0, pz - render_radius); cz <= std::min(grid_h_ - 1, pz + render_radius); ++cz) {
        for (int cx = std::max(0, px - render_radius); cx <= std::min(grid_w_ - 1, px + render_radius); ++cx) {
            // Floor tile
            glm::mat4 floor_m = glm::translate(glm::mat4(1.0f), glm::vec3(cx + 0.5f, -0.5f, cz + 0.5f));
            glm::vec3 floor_col = ((cx + cz) % 2 == 0) ? glm::vec3(0.24f, 0.25f, 0.28f) : glm::vec3(0.18f, 0.19f, 0.22f);
            drawCube(floor_m, floor_col);

            // Ceiling tile
            glm::mat4 ceil_m = glm::translate(glm::mat4(1.0f), glm::vec3(cx + 0.5f, 2.5f, cz + 0.5f));
            glm::vec3 ceil_col(0.15f, 0.16f, 0.19f);
            drawCube(ceil_m, ceil_col);
        }
    }

    // 3. Draw Collectible Keys (Hovering & spinning 3D diamonds)
    for (const auto& k : keys) {
        if (!k.collected) {
            float hover_y = k.position.y + 0.15f * std::sin(animation_time_ * 3.0f);
            glm::mat4 key_m = glm::translate(glm::mat4(1.0f), glm::vec3(k.position.x, hover_y, k.position.z));
            key_m = glm::rotate(key_m, animation_time_ * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            key_m = glm::rotate(key_m, glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 1.0f));
            key_m = glm::scale(key_m, glm::vec3(0.35f, 0.35f, 0.35f));
            drawCube(key_m, k.color);
        }
    }

    // 4. Draw Exit Portal Gate
    glm::mat4 exit_m = glm::translate(glm::mat4(1.0f), exit_pos);
    exit_m = glm::scale(exit_m, glm::vec3(0.8f, 1.8f, 0.8f));
    glm::vec3 exit_color = exit_unlocked ? glm::vec3(0.1f, 0.95f, 0.3f) : glm::vec3(0.95f, 0.15f, 0.15f);
    // Add pulsing glow effect
    float pulse = 0.85f + 0.15f * std::sin(animation_time_ * 4.0f);
    drawCube(exit_m, exit_color * pulse);

    // 5. Draw Chasing Monsters (Red animated predator cubes with glowing eyes)
    for (const auto& m : monster_mgr.monsters) {
        float bob = 0.05f * std::sin(animation_time_ * 6.0f);
        glm::mat4 m_mat = glm::translate(glm::mat4(1.0f), glm::vec3(m.position.x, m.position.y + bob, m.position.z));
        float angle = std::atan2(m.forward.x, m.forward.z);
        m_mat = glm::rotate(m_mat, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        m_mat = glm::scale(m_mat, glm::vec3(0.55f, 0.75f, 0.55f));

        glm::vec3 monster_col = (m.state == MonsterState::CHASE) ? glm::vec3(0.95f, 0.1f, 0.1f) : glm::vec3(0.7f, 0.2f, 0.2f);
        drawCube(m_mat, monster_col);

        // Glowing Monster Eye
        glm::mat4 eye_m = glm::translate(m_mat, glm::vec3(0.0f, 0.2f, 0.55f));
        eye_m = glm::scale(eye_m, glm::vec3(0.3f, 0.15f, 0.1f));
        drawCube(eye_m, glm::vec3(1.0f, 0.9f, 0.1f));
    }

    // 6. Draw Solution Breadcrumb Waypoints (if toggled)
    if (player.show_path_hint && !hint_path.empty()) {
        glm::vec3 crumb_col(0.1f, 0.85f, 0.95f);
        for (size_t i = 0; i < hint_path.size(); i += 2) {
            int node = hint_path[i];
            int cx = node % maze_w;
            int cy = node / maze_w;
            int gx = 2 * cx + 1;
            int gz = 2 * cy + 1;
            glm::mat4 crumb_m = glm::translate(glm::mat4(1.0f), glm::vec3(gx + 0.5f, 0.15f, gz + 0.5f));
            crumb_m = glm::scale(crumb_m, glm::vec3(0.18f, 0.18f, 0.18f));
            drawCube(crumb_m, crumb_col);
        }
    }

    glBindVertexArray(0);
}

void Renderer::drawQuad2D(float x, float y, float w, float h, const glm::vec4& color) {
    glUniform2f(glGetUniformLocation(hud_shader_, "position"), x, y);
    glUniform2f(glGetUniformLocation(hud_shader_, "size"), w, h);
    glUniform4fv(glGetUniformLocation(hud_shader_, "color"), 1, glm::value_ptr(color));
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::renderHUD(const Player& player,
                         const MonsterManager& monster_mgr,
                         const std::vector<KeyItem>& keys,
                         const std::vector<std::string>& block_grid,
                         float game_time,
                         float fps,
                         const std::string& status_msg,
                         bool is_time_trial,
                         float time_remaining)
{
    (void)game_time;
    (void)fps;
    (void)status_msg;
    (void)is_time_trial;
    (void)time_remaining;

    glDisable(GL_DEPTH_TEST);
    glUseProgram(hud_shader_);

    glm::mat4 ortho = glm::ortho(0.0f, (float)screen_width_, 0.0f, (float)screen_height_);
    glUniformMatrix4fv(glGetUniformLocation(hud_shader_, "projection"), 1, GL_FALSE, glm::value_ptr(ortho));

    glBindVertexArray(quad_vao_);

    // 1. Crosshair in center
    float cx = screen_width_ * 0.5f;
    float cy = screen_height_ * 0.5f;
    drawQuad2D(cx - 1.0f, cy - 8.0f, 2.0f, 16.0f, glm::vec4(1.0f, 1.0f, 1.0f, 0.7f));
    drawQuad2D(cx - 8.0f, cy - 1.0f, 16.0f, 2.0f, glm::vec4(1.0f, 1.0f, 1.0f, 0.7f));

    // 2. Health Bar (Bottom-Left)
    float hp_ratio = std::max(0.0f, (float)player.health / player.max_health);
    drawQuad2D(28.0f, 38.0f, 204.0f, 20.0f, glm::vec4(0.08f, 0.1f, 0.12f, 0.85f)); // BG
    glm::vec4 hp_col = (hp_ratio > 0.4f) ? glm::vec4(0.15f, 0.85f, 0.35f, 0.95f) : glm::vec4(0.95f, 0.2f, 0.2f, 0.95f);
    drawQuad2D(30.0f, 40.0f, 200.0f * hp_ratio, 16.0f, hp_col);

    // 3. Stamina Bar (Above Health)
    float st_ratio = std::max(0.0f, player.stamina / player.max_stamina);
    drawQuad2D(28.0f, 64.0f, 164.0f, 12.0f, glm::vec4(0.08f, 0.1f, 0.12f, 0.85f));
    drawQuad2D(30.0f, 66.0f, 160.0f * st_ratio, 8.0f, glm::vec4(0.1f, 0.75f, 0.95f, 0.95f));

    // 4. Key Badges (Bottom-Left above stamina)
    // Bronze
    glm::vec4 b_col = player.hasKey(KEY_BRONZE) ? glm::vec4(0.85f, 0.5f, 0.2f, 1.0f) : glm::vec4(0.3f, 0.3f, 0.3f, 0.5f);
    drawQuad2D(30.0f, 86.0f, 20.0f, 20.0f, b_col);
    // Silver
    glm::vec4 s_col = player.hasKey(KEY_SILVER) ? glm::vec4(0.85f, 0.85f, 0.9f, 1.0f) : glm::vec4(0.3f, 0.3f, 0.3f, 0.5f);
    drawQuad2D(56.0f, 86.0f, 20.0f, 20.0f, s_col);
    // Gold
    glm::vec4 g_col = player.hasKey(KEY_GOLD) ? glm::vec4(1.0f, 0.85f, 0.1f, 1.0f) : glm::vec4(0.3f, 0.3f, 0.3f, 0.5f);
    drawQuad2D(82.0f, 86.0f, 20.0f, 20.0f, g_col);

    // 5. Minimap Overlay (Top-Right)
    if (!block_grid.empty()) {
        float map_size = 180.0f;
        float map_x = screen_width_ - map_size - 24.0f;
        float map_y = screen_height_ - map_size - 24.0f;

        // Minimap background frame
        drawQuad2D(map_x - 3.0f, map_y - 3.0f, map_size + 6.0f, map_size + 6.0f, glm::vec4(0.05f, 0.08f, 0.12f, 0.9f));

        int gh = static_cast<int>(block_grid.size());
        int gw = static_cast<int>(block_grid[0].size());
        float cell_w = map_size / gw;
        float cell_h = map_size / gh;

        for (int z = 0; z < gh; ++z) {
            for (int x = 0; x < gw; ++x) {
                if (block_grid[z][x] == '#') {
                    drawQuad2D(map_x + x * cell_w, map_y + (gh - 1 - z) * cell_h, cell_w, cell_h, glm::vec4(0.35f, 0.38f, 0.44f, 0.8f));
                }
            }
        }

        // Draw Keys on Minimap
        for (const auto& k : keys) {
            if (!k.collected) {
                float kx = map_x + k.position.x * cell_w;
                float kz = map_y + (gh - 1 - k.position.z) * cell_h;
                drawQuad2D(kx - 2.0f, kz - 2.0f, 5.0f, 5.0f, glm::vec4(k.color, 1.0f));
            }
        }

        // Draw Monsters on Minimap (red blips)
        for (const auto& m : monster_mgr.monsters) {
            float mx = map_x + m.position.x * cell_w;
            float mz = map_y + (gh - 1 - m.position.z) * cell_h;
            drawQuad2D(mx - 2.5f, mz - 2.5f, 6.0f, 6.0f, glm::vec4(0.95f, 0.1f, 0.1f, 1.0f));
        }

        // Draw Player position & orientation
        float p_mx = map_x + player.position.x * cell_w;
        float p_mz = map_y + (gh - 1 - player.position.z) * cell_h;
        drawQuad2D(p_mx - 3.0f, p_mz - 3.0f, 7.0f, 7.0f, glm::vec4(0.15f, 0.95f, 0.4f, 1.0f));

        // Player orientation tick
        float arrow_x = p_mx + player.front.x * 7.0f;
        float arrow_z = p_mz - player.front.z * 7.0f;
        drawQuad2D(arrow_x - 1.5f, arrow_z - 1.5f, 3.0f, 3.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    glBindVertexArray(0);
}
