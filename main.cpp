#include <cmath>
#include <string>
#include <random>
#include <iostream>
#include <math.h>
#include <cstdlib>

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
void render(std::vector<GLuint> &map_chunks, Shader &shader, glm::mat4 &view, glm::mat4 &model, glm::mat4 &projection, glm::vec3 &lightPos, int &nIndices);

std::vector<int> generate_indices();
std::vector<float> generate_noise_map(int xOffset, int yOffset);
std::vector<float> generate_vertices(const std::vector<float> &noise_map);
std::vector<float> generate_normals(const std::vector<int> &indices, const std::vector<float> &vertices);
std::vector<float> generate_colors(const std::vector<float> &vertices);
void generate_map_chunk(GLuint &VAO, int xOffset, int yOffset);

GLFWwindow *window;

// Map params
float WATER_HEIGHT = 0.1;
int xMapChunks = 5;
int yMapChunks = 5;
int chunkWidth = 127;
int chunkHeight = 127;

// Noise params
int octaves = 5;
float meshHeight = 32;  // Vertical scaling
float noiseScale = 64;  // Horizontal scaling
float persistence = 0.5;
float lacunarity = 2;

// Camera
Camera camera(glm::vec3((chunkWidth * xMapChunks) / 2, 20.0f, (chunkHeight * yMapChunks) / 2));
bool firstMouse = true;
float lastX = WIDTH / 2;
float lastY = HEIGHT / 2;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float currentFrame;

int main() {
    // Initalize variables
    glm::mat4 view;
    glm::mat4 model;
    glm::mat4 projection;
    glm::vec3 lightPos;

    // Initialize GLFW and GLAD
    if (init() != 0)
        return -1;
    
    Shader shader("vshader.vs", "fshader.fs");
    
    // Lighting
    shader.use();
    shader.setVec3("u_lightColor", 1.0f, 1.0f, 1.0f);
    
    // Default to flat mode
    shader.setBool("isFlat", true);
    
    std::vector<GLuint> map_chunks(xMapChunks * yMapChunks);
    
    for (int y = 0; y < yMapChunks; y++)
        for (int x = 0; x < xMapChunks; x++) {
            generate_map_chunk(map_chunks[x + y*xMapChunks], x, y);
        }
    
    int nIndices = chunkWidth * chunkHeight * 6;
    
    while (!glfwWindowShouldClose(window)) {
        render(map_chunks, shader, view, model, projection, lightPos, nIndices);
    }
    
    for (int i = 0; i < map_chunks.size(); i++)
        glDeleteVertexArrays(1, &map_chunks[i]);
    
    // TODO VBOs and EBOs aren't being deleted
    // glDeleteBuffers(3, VBO);
    // glDeleteBuffers(1, &EBO);
    
    glfwTerminate();
    
    return 0;
}

void generate_map_chunk(GLuint &VAO, int xOffset, int yOffset) {
    std::vector<int> indices;
    std::vector<float> noise_map;
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> colors;
    
    // Generate map
    indices = generate_indices();
    noise_map = generate_noise_map(xOffset, yOffset);
    vertices = generate_vertices(noise_map);
    normals = generate_normals(indices, vertices);
    colors = generate_colors(vertices);
    
    GLuint VBO[3], EBO;
    
    // Create buffers and arrays
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
}

void render(std::vector<GLuint> &map_chunks, Shader &shader, glm::mat4 &view, glm::mat4 &model, glm::mat4 &projection, glm::vec3 &lightPos, int &nIndices) {
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
    // Last value is draw distance
    projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 700.0f);
    shader.setMat4("u_projection", projection);
    view = camera.GetViewMatrix();
    shader.setMat4("u_view", view);
    
    // Set view position
    shader.setVec3("u_viewPos", camera.Position);
    
    // Dynamic lighting
    lightPos = glm::vec3((chunkWidth * xMapChunks) / 2 + glm::sin(0.25*glfwGetTime()) * 250, 50.0, (chunkHeight * yMapChunks) / 2 +  glm::cos(0.25*glfwGetTime()) * 250);
    shader.setVec3("u_lightPos", lightPos);
    
    // Render chunks
    for (int y = 0; y < yMapChunks; y++)
        for (int x = 0; x < xMapChunks; x++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-chunkWidth / 2.0 + (chunkWidth - 1) * x, 0.0, -chunkHeight / 2.0 + (chunkHeight - 1) * y));
            shader.setMat4("u_model", model);
            shader.setVec3("u_objectColor", glm::vec3(0.2, 0.3, 0.6));
            
            glBindVertexArray(map_chunks[x + y*xMapChunks]);
            glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
        }
    
    // Use double buffer
    // Only swap old frame with new when it is completed
    glfwPollEvents();
    glfwSwapBuffers(window);
}

glm::vec3 get_color(int r, int g, int b) {
    return glm::vec3(r/255.0, g/255.0, b/255.0);
}

std::vector<float> generate_noise_map(int offsetX, int offsetY) {
    std::vector<float> noiseValues;
    std::vector<float> normalizedNoiseValues;
    std::vector<int> p = get_permutation_vector();
    
    float amp  = 1;
    float freq = 1;
    float maxPossibleHeight = 0;
    
    for (int i = 0; i < octaves; i++) {
        maxPossibleHeight += amp;
        amp *= persistence;
    }
    
    for (int y = 0; y < chunkHeight; y++) {
        for (int x = 0; x < chunkWidth; x++) {
            amp  = 1;
            freq = 1;
            float noiseHeight = 0;
            for (int i = 0; i < octaves; i++) {
                float xSample = (x + offsetX * (chunkWidth-1))  / noiseScale * freq;
                float ySample = (y + offsetY * (chunkHeight-1)) / noiseScale * freq;
                
                float perlinValue = perlin_noise(xSample, ySample, p);
                noiseHeight += perlinValue * amp;
                
                // Lacunarity  --> Increase in frequency of octaves
                // Persistence --> Decrease in amplitude of octaves
                amp  *= persistence;
                freq *= lacunarity;
            }
            
            noiseValues.push_back(noiseHeight);
        }
    }
    
    for (int y = 0; y < chunkHeight; y++) {
        for (int x = 0; x < chunkWidth; x++) {
            // Inverse lerp and scale values to range from 0 to 1
            normalizedNoiseValues.push_back((noiseValues[x + y*chunkWidth] + 1) / maxPossibleHeight);
        }
    }

    return normalizedNoiseValues;
}

struct terrainColor {
    terrainColor(float _height, glm::vec3 _color) {
        height = _height;
        color = _color;
    };
    float height;
    glm::vec3 color;
};

std::vector<float> generate_colors(const std::vector<float> &vertices) {
    std::vector<float> colors;
    std::vector<terrainColor> biomeColors;
    glm::vec3 color = get_color(255, 255, 255);
    
    // NOTE: Terrain color height is a value between 0 and 1
    biomeColors.push_back(terrainColor(WATER_HEIGHT * 0.5, get_color(60,  95, 190)));  // Deep water
    biomeColors.push_back(terrainColor(WATER_HEIGHT,        get_color(60, 100, 190)));  // Shallow water
    biomeColors.push_back(terrainColor(0.15, get_color(210, 215, 130)));                // Sand
    biomeColors.push_back(terrainColor(0.30, get_color( 95, 165,  30)));                // Grass 1
    biomeColors.push_back(terrainColor(0.40, get_color( 65, 115,  20)));                // Grass 2
    biomeColors.push_back(terrainColor(0.50, get_color( 90,  65,  60)));                // Rock 1
    biomeColors.push_back(terrainColor(0.80, get_color( 75,  60,  55)));                // Rock 2
    biomeColors.push_back(terrainColor(1.00, get_color(255, 255, 255)));                // Snow
    
    // Determine which color to assign each vertex by its y-coord
    // Iterate through vertex y values
    for (int i = 1; i < vertices.size(); i += 3) {
        for (int j = 0; j < biomeColors.size(); j++) {
            // NOTE: The max height of a vertex is "meshHeight"
            if (vertices[i] <= biomeColors[j].height * meshHeight) {
                color = biomeColors[j].color;
                break;
            }
        }
        colors.push_back(color.r);
        colors.push_back(color.g);
        colors.push_back(color.b);
    }
    
    return colors;
}

std::vector<float> generate_normals(const std::vector<int> &indices, const std::vector<float> &vertices) {
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

std::vector<float> generate_vertices(const std::vector<float> &noise_map) {
    std::vector<float> v;
    
    for (int y = 0; y < chunkHeight + 1; y++)
        for (int x = 0; x < chunkWidth; x++) {
            v.push_back(x);
            // Apply cubic easing to the noise
            float easedNoise = std::pow(noise_map[x + y*chunkWidth] * 1.1, 3);
            // Scale noise to match meshHeight
            // Pervent vertex height from being below WATER_HEIGHT
            v.push_back(std::fmax(easedNoise * meshHeight, WATER_HEIGHT * 0.5 * meshHeight));
            v.push_back(y);
        }
    
    return v;
}

std::vector<int> generate_indices() {
    std::vector<int> indices;
    
    for (int y = 0; y < chunkHeight; y++)
        for (int x = 0; x < chunkWidth; x++) {
            int pos = x + y*chunkWidth;
            
            if (x == chunkWidth - 1 || y == chunkHeight - 1) {
                // Don't create indices for right or top edge
                continue;
            } else {
                // Top left triangle of square
                indices.push_back(pos + chunkWidth);
                indices.push_back(pos);
                indices.push_back(pos + chunkWidth + 1);
                // Bottom right triangle of square
                indices.push_back(pos + 1);
                indices.push_back(pos + 1 + chunkWidth);
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
