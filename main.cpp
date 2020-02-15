#include <cmath>
#include <string>
#include <random>
#include <iostream>
#include <math.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
#include "perlin.h"

const GLint WIDTH = 1920, HEIGHT = 1080;

// Functions
int init();
void processInput(GLFWwindow *window, Shader shader);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void render(const GLuint &VAO, Shader &shader, glm::mat4 &view, glm::mat4 &model, glm::mat4 &projection, glm::vec3 &lightPos, int &map_width, int &map_height, int &nIndices);

std::vector<int> generate_indices(int width, int height);
std::vector<float> generate_noise_map(int width, int height);
std::vector<float> generate_vertices(int width, int height, const std::vector<float> &noise_map);
std::vector<float> generate_normals(int width, int height, const std::vector<int> &indices, const std::vector<float> &vertices);
std::vector<float> generate_colors(const std::vector<float> &vertices);

// Camera
Camera camera(glm::vec3(0.0f, 20.0f, 0.0f));
bool firstMouse = true;
float lastX = WIDTH / 2;
float lastY = HEIGHT / 2;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float currentFrame;

// Misc globals
GLFWwindow *window;

int main() {
    // Initalize variables
    int map_width = 127;
    int map_height = 127;
    
    glm::mat4 view;
    glm::mat4 model;
    glm::mat4 projection;
    glm::vec3 lightPos;
    
    std::vector<int> indices;
    std::vector<float> noise_map;
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> colors;

    // Initialize GLFW and GLAD
    if (init() != 0)
        return -1;
    
    Shader shader("vshader.vs", "fshader.fs");
    
    // Generate map
    indices = generate_indices(map_width, map_height);
    noise_map = generate_noise_map(map_width, map_height);
    vertices = generate_vertices(map_width, map_height, noise_map);
    normals = generate_normals(map_width, map_height, indices, vertices);
    colors = generate_colors(vertices);
    
    // Create buffers and arrays
    GLuint VAO, VBO[3], EBO;
    glGenBuffers(3, VBO);
    glGenBuffers(1, &EBO);
    glGenVertexArrays(1, &VAO);
    
    // Bind vertices to VBO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    
    // Create element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);
    
    // Configure vertex position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Bind vertices to VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &normals[0], GL_STATIC_DRAW);
    
    // Configure vertex normals attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    
    // Bind vertices to VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), &colors[0], GL_STATIC_DRAW);
    
    // Configure vertex colors attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    
    // Lighting
    shader.use();
    shader.setVec3("u_lightColor", 1.0f, 0.8f, 0.8f);
    
    // Default to flat mode
    shader.setBool("isFlat", true);
    
    int nIndices = (int)indices.size();
    
    while (!glfwWindowShouldClose(window)) {
        render(VAO, shader, view, model, projection, lightPos, map_width, map_height, nIndices);
    }
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(3, VBO);
    glDeleteBuffers(1, &EBO);
    
    glfwTerminate();
    
    return 0;
}

void render(const GLuint &VAO, Shader &shader, glm::mat4 &view, glm::mat4 &model, glm::mat4 &projection, glm::vec3 &lightPos, int &map_width, int &map_height, int &nIndices) {
    // Per-frame time logic
    currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    processInput(window, shader);
    
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Activate shader
    shader.use();

    // Set projection and view matrix
    projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
    shader.setMat4("u_projection", projection);
    view = camera.GetViewMatrix();
    shader.setMat4("u_view", view);
    
    // Set view position
    shader.setVec3("u_viewPos", camera.Position);
    
    // Dynamic lighting
    lightPos = glm::vec3(glm::sin(0.5*glfwGetTime()) * 100, 20.0, glm::cos(0.5*glfwGetTime()) * 100);
    shader.setVec3("u_lightPos", lightPos);
    
    // Render terrain
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-map_width / 2.0, 0.0, -map_height / 2.0));
    shader.setMat4("u_model", model);
    shader.setVec3("u_objectColor", glm::vec3(0.2, 0.3, 0.6));
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
    
    // Use double buffer
    // Only swap old frame with new when it is completed
    glfwPollEvents();
    glfwSwapBuffers(window);
}

glm::vec3 get_color(int r, int g, int b) {
    return glm::vec3(r/255.0, g/255.0, b/255.0);
}

std::vector<float> generate_colors(const std::vector<float> &vertices) {
    std::vector<float> colors;
    std::vector<glm::vec3> biomeColors;
    glm::vec3 color;
    
    // Spread is the amount of alitude range colors are spread over
    // This accounts for the long tails of perlin noise
    float spread = 0.45f;
    float halfSpread = spread / 2;
    
    // Amplitude is the max height of vertices
    float amplitude = 0;
    for (int y_val = 1; y_val < vertices.size(); y_val += 3)
        if (vertices[y_val] > amplitude)
            amplitude = vertices[y_val];
    
    biomeColors.push_back(get_color(201, 178, 99));
    biomeColors.push_back(get_color(135, 184, 82));
    biomeColors.push_back(get_color(80, 171, 93));
    biomeColors.push_back(get_color(120, 120, 120));
    biomeColors.push_back(get_color(200, 200, 210));

    float part = 1.0f / (biomeColors.size() - 1);

    // Iterate through vertex y values
    for (int y_val = 1; y_val < vertices.size(); y_val += 3) {
        float height = vertices[y_val];
        float value = (height + amplitude) / (amplitude * 2);
        
        value = std::max(0.0f, std::min((value - halfSpread) * (1 / spread), 0.9999f));
        int firstBiome = (int)floor(value / part);
        float blend = (value - (firstBiome * part)) / part;

        // Lerp between the two biome colors
        color = (1 - blend) * biomeColors[firstBiome] + blend * biomeColors[firstBiome+1];
        colors.push_back(color.r);
        colors.push_back(color.g);
        colors.push_back(color.b);
    }
    
    return colors;
}

std::vector<float> generate_normals(int width, int height, const std::vector<int> &indices, const std::vector<float> &vertices) {
    int pos;
    glm::vec3 normal;
    std::vector<float> normals;
    std::vector<glm::vec3> verts;
    
    // Get the vertices of each triangle in mesh
    // For each group of indices
    for (int i = 0; i < indices.size(); i += 3) {
        
        // Get the vertices (point) for each index
        for (int j = 0; j < 3; j++) {
            pos = indices[i+j]*3;
            verts.push_back(glm::vec3(vertices[pos], vertices[pos+1], vertices[pos+2]));
        }
        
        // Get vectors of two edges of triangle
        glm::vec3 U = verts[i+1] - verts[i];
        glm::vec3 V = verts[i+2] - verts[i];
        
        // Calculate normal
        normal = glm::normalize(glm::cross(U, V));
        normals.push_back(normal.x);
        normals.push_back(normal.y);
        normals.push_back(normal.z);
    }
    
    return normals;
}

std::vector<float> generate_noise_map(int width, int height) {
    std::vector<float> noise_map;
    
    int seed = 27;
    int octaves = 3;
    double frequency = 12;
    
    siv::PerlinNoise p(seed);
    
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) {
            noise_map.push_back(p.octaveNoise0_1(x / frequency, y / frequency, octaves));
        }
    
    return noise_map;
}

std::vector<float> generate_vertices(int width, int height, const std::vector<float> &noise_map) {
    std::vector<float> v;
    float scale = 1.0;
    
    for (int y = 0; y < height + 1; y++)
        for (int x = 0; x < width; x++) {
            v.push_back(x*scale);
            v.push_back(noise_map[x + y*width]);
            v.push_back(y*scale);
        }
    
    return v;
}

std::vector<int> generate_indices(int width, int height) {
    std::vector<int> indices;
    
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) {
            int pos = x + y*width;
            
            if (x == width - 1 || y == height - 1) {
                // Don't create indices for right or top edge
                continue;
            } else {
                // Top left triangle of square
                indices.push_back(pos + width);
                indices.push_back(pos);
                indices.push_back(pos + width + 1);
                // Bottom right triangle of square
                indices.push_back(pos + 1);
                indices.push_back(pos + 1 + width);
                indices.push_back(pos);
            }
        }

    return indices;
}

// Initialize GLFW and GLAD
int init() {
    glfwInit();
    
    // Set OpenGL window to version 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // macOS compatibility
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    window = glfwCreateWindow(WIDTH, HEIGHT, "Terrain Generator", nullptr, nullptr);
    
    // Account for macOS retina resolution
    int screenWidth, screenHeight;
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, screenWidth, screenHeight);

    // Enable z-buffer
    glEnable(GL_DEPTH_TEST);
    
    // Enable mouse input
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    return 0;
}

void processInput(GLFWwindow *window, Shader shader) {
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Enable wireframe mode
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    // Enable flat mode
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        shader.use();
        shader.setBool("isFlat", false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        shader.use();
        shader.setBool("isFlat", true);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    // Prevent camera jumping when mouse first enters screen
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    // yoffset is reversed since y-coords go from bottom to top
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}
