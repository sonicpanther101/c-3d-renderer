#include <GL/glew.h>
#include <GLFW/glfw3.h>

// GLM
#include "../vendor/glm/glm/glm.hpp"
#include "../vendor/glm/glm/gtc/matrix_transform.hpp"
#include "../vendor/glm/glm/gtc/type_ptr.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "shader.h"
#include "camera.h"
#include "model.h"
#include "physics.h"

#include <iostream>
#include <vector>
#include <random>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, PhysicsSystem &physicsSystem);
glm::vec3 hueToRGB(float hue);
unsigned int loadTexture(char const * path);
void printVec3(const glm::vec3& v);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

bool mouseEnabled = false;
bool CPressed = false;
bool wireframe = false;

// camera
Camera camera(glm::vec3(2.0f, 0.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float scale = 0.2f;

// timing
float deltaTime = 0.0f;	
float lastFrame = 0.0f;

// lighting
glm::vec3 pointLightPositions[] = {
	glm::vec3( 0.7f,  0.2f,  2.0f),
	glm::vec3( 2.3f, -3.3f, -4.0f),
	glm::vec3(-4.0f,  2.0f, -12.0f),
	glm::vec3( 0.0f,  0.0f, -3.0f)
};

float corners[] = {
   -0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f,  0.5f, -0.5f,
   -0.5f,  0.5f, -0.5f,

   -0.5f, -0.5f,  0.5f,
    0.5f, -0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
   -0.5f,  0.5f,  0.5f
};

std::vector<int> edgeConstraints[] = {
    // back
    {0, 1},
    {1, 2},
    {2, 3},
    {3, 0},
    // front
    {4, 5},
    {5, 6},
    {6, 7},
    {7, 4},
    // middle
    {0, 4},
    {1, 5},
    {2, 6},
    {3, 7},
    // diagonals
    {0, 2},
    {0, 5},
    {0, 7},
    {1, 6},
    {3, 6},
    {4, 6}
};

int main() {

    std::vector<PhysicsSystem::Particle> vertices;
    std::vector<PhysicsSystem::Constraint> edges;

    for (int i = 0; i < 24; i+=3) {
        vertices.push_back(PhysicsSystem::Particle(vertices.size(), glm::vec3(corners[i], corners[i+1], corners[i+2]), glm::vec3(0.0f)));
    }
    for (int i = 0; i < 24; i+=3) {
        vertices.push_back(PhysicsSystem::Particle(vertices.size(), glm::vec3(corners[i], corners[i+1]+1.5f, corners[i+2]), glm::vec3(0.0f)));
    }
    vertices[0].velocity = glm::vec3(0.0f, 0.0f, 10.0f);


    for (int j = 0; j < 12; j++) {
        edges.push_back(PhysicsSystem::Constraint(edgeConstraints[j]));
    }
    for (int j = 12; j < 18; j++) {
        edges.push_back(PhysicsSystem::Constraint(
            edgeConstraints[j], // indices
            0.98f,              // stiffness
            false,              // equality
            [](std::vector<PhysicsSystem::Particle*> particles) -> float {
                if (particles.size() != 2) {
                    std::cout << "Constraint for that dimension is not supported yet" << std::endl;
                    return 0.0f;
                }
                glm::vec3 difference = particles[0]->position - particles[1]->position;
                return glm::length(difference) - glm::sqrt(2);
            },
            [](std::vector<PhysicsSystem::Particle*> particles) -> std::vector<glm::vec3> {
                if (particles.size() != 2) {
                    std::cout << "Constraint for that dimension is not supported yet" << std::endl;
                    return {glm::vec3(0.0f), glm::vec3(0.0f)};
                }

                glm::vec3 difference = particles[0]->position - particles[1]->position;
                float length = glm::length(difference);

                if (length < EPSILON) {
                    return {glm::vec3(0.0f), glm::vec3(0.0f)};
                }

                glm::vec3 normalized = difference / length;
                return {normalized, -normalized};
            }
        ));
    }
    for (int j = 0; j < 12; j++) {
        edges.push_back(PhysicsSystem::Constraint({edgeConstraints[j][0]+8, edgeConstraints[j][1]+8}));
    }
    for (int j = 12; j < 18; j++) {
        edges.push_back(PhysicsSystem::Constraint(
            {edgeConstraints[j][0]+8, edgeConstraints[j][1]+8}, // indices
            0.98f,                                              // stiffness
            false,                                              // equality
            [](std::vector<PhysicsSystem::Particle*> particles) -> float {
                if (particles.size() != 2) {
                    std::cout << "Constraint for that dimension is not supported yet" << std::endl;
                    return 0.0f;
                }
                glm::vec3 difference = particles[0]->position - particles[1]->position;
                return glm::length(difference) - glm::sqrt(2);
            },
            [](std::vector<PhysicsSystem::Particle*> particles) -> std::vector<glm::vec3> {
                if (particles.size() != 2) {
                    std::cout << "Constraint for that dimension is not supported yet" << std::endl;
                    return {glm::vec3(0.0f), glm::vec3(0.0f)};
                }

                glm::vec3 difference = particles[0]->position - particles[1]->position;
                float length = glm::length(difference);

                if (length < EPSILON) {
                    return {glm::vec3(0.0f), glm::vec3(0.0f)};
                }

                glm::vec3 normalized = difference / length;
                return {normalized, -normalized};
            }
        ));
    }

    // for (int i = 0; i < 16; i++) {
    //     edges.push_back(PhysicsSystem::Constraint(
    //         {i},  // indices
    //         0.98f,              // stiffness
    //         false,             // inequality
    //         [](auto particles) -> float {
    //             return particles[0]->position.y;
    //         },
    //         [](auto particles) -> std::vector<glm::vec3> {
    //             if (particles.size() != 1) {
    //                 std::cerr << "Expected 1 particle in gradient, got " << particles.size() << "\n";
    //                 return {glm::vec3(0,0,0)};
    //             }
    //             return std::vector<glm::vec3>{glm::vec3(0,1,0)};
    //         }
    //     ));
    // }

    // for rendering cube
    std::vector<unsigned int> edgeIndices;
    for (int i = 0; i < 18; i++) {
        edgeIndices.push_back(edgeConstraints[i][0]);
        edgeIndices.push_back(edgeConstraints[i][1]);
    }
    for (int i = 0; i < 18; i++) {
        edgeIndices.push_back(edgeConstraints[i][0]+8);
        edgeIndices.push_back(edgeConstraints[i][1]+8);
    }
    
    PhysicsSystem system(vertices, edges);

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSwapInterval(0);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glew: load all OpenGL function pointers
    // ---------------------------------------
    GLenum err = glewInit();
	if (err != GLEW_OK) {
		std::cout << "Failed to initialize GLEW: %s" << glewGetErrorString(err) << std::endl;
        return -1;
	}

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // render 1 frame to stop flashbang startup
    processInput(window, system);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwPollEvents();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int billboardVAO, billboardVBO, pointsVBO, radiiVBO;
    const float quadVertices[] = {
        // Positions
        -0.5f,  0.5f,
        -0.5f, -0.5f,
        0.5f, -0.5f,
        -0.5f,  0.5f,
        0.5f, -0.5f,
        0.5f,  0.5f
    };

    std::vector<unsigned int> faceIndices = {
        // Back face
        0, 1, 2, 2, 3, 0,
        // Front face
        4, 5, 6, 6, 7, 4,
        // Left face
        0, 3, 7, 7, 4, 0,
        // Right face
        1, 5, 6, 6, 2, 1,
        // Bottom face
        0, 4, 5, 5, 1, 0,
        // Top face
        3, 2, 6, 6, 7, 3
    };
    for (int i = 0; i < 36; i++) {
        faceIndices.push_back(faceIndices[i] + 8);
    }

    // build and compile our shader program
    // ------------------------------------
    Shader pointShader("../shaders/point_vertex.glsl", "../shaders/point_fragment.glsl");
    Shader lineShader("../shaders/line_vertex.glsl", "../shaders/line_fragment.glsl");
    Shader cubeShader("../shaders/cube_vertex.glsl", "../shaders/cube_fragment.glsl");

    pointShader.use();

    pointShader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));

    cubeShader.use();
    cubeShader.setVec3("lightPos", glm::vec3(5.0f, 5.0f, 5.0f));
    cubeShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    cubeShader.setVec3("objectColor", glm::vec3(0.0f, 0.0f, 1.0f));
    // world transformation
    glm::mat4 model = glm::mat4(1.0f);
    cubeShader.setMat4("model", model);

    glGenVertexArrays(1, &billboardVAO);
    glGenBuffers(1, &billboardVBO);
    glGenBuffers(1, &pointsVBO);
    glGenBuffers(1, &radiiVBO);

    glBindVertexArray(billboardVAO);

    // Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, billboardVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // Instance buffer
    glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
    glBufferData(GL_ARRAY_BUFFER, 100 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(1, 1);  // Update once per instance

    glBindVertexArray(0);

    // rendering cube

    unsigned int cubeVAO, cubelinesVAO, cubeVBO, cubelinesVBO, cubeEBO, cubelinesEBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenVertexArrays(1, &cubelinesVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubelinesVBO);
    glGenBuffers(1, &cubeEBO);
    glGenBuffers(1, &cubelinesEBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, faceIndices.size() * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, faceIndices.size() * sizeof(unsigned int), faceIndices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);

    // rendering cube lines

    glBindVertexArray(cubelinesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubelinesVBO);
    glBufferData(GL_ARRAY_BUFFER, edgeIndices.size() * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubelinesEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, edgeIndices.size() * sizeof(unsigned int), edgeIndices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);

    // Imgui stuff
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    #if defined(__linux__)
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    #else
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
    #endif

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

	system.Start();
	
    // render loop
    // -----------

    while (!glfwWindowShouldClose(window)) {

        if (wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else 
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // std::cout << 1/deltaTime << std::endl; // fps console log

        std::vector<glm::vec3> positions;
        system.GetParticlePositions(positions);

        std::vector<float> sizes;
        system.GetParticleSizes(sizes);

        // Update instance data
        glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(glm::vec3), positions.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Update cube positions
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(glm::vec3), positions.data());

        // Update cube line positions
        glBindBuffer(GL_ARRAY_BUFFER, cubelinesVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(glm::vec3), positions.data());


        // input
        // -----
        processInput(window, system);

        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        pointShader.setMat4("projection", projection);
        pointShader.setMat4("view", view);

        // Draw cube

        // Solid triangles
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        cubeShader.use();
        cubeShader.setMat4("projection", projection);
        cubeShader.setMat4("view", view);
        glBindVertexArray(cubeVAO);
        glDrawElements(GL_TRIANGLES, faceIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glDisable(GL_BLEND);

        // Lines
        glLineWidth(4.0f); // Make lines thicker
        lineShader.use();
        lineShader.setMat4("projection", projection);
        lineShader.setMat4("view", view);
        glBindVertexArray(cubelinesVAO);
        glDrawElements(GL_LINES, edgeIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // be sure to activate shader when setting uniforms/drawing objects
        pointShader.use();

        pointShader.setVec3("viewPos", camera.Position);
        pointShader.setFloat("time", static_cast<float>(glfwGetTime()));

        // Billboard-specific uniforms
        pointShader.setVec3("cameraRight", camera.Right);
        pointShader.setVec3("cameraUp", camera.Up);
        pointShader.setFloat("billboardScale", scale);
        // pointShader.setFloat("billboardScale", 696340e3f);

        // Draw billboards
        glBindVertexArray(billboardVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, positions.size());

        // ImGui
        ImGui::Begin("Changer");
        ImGui::SliderFloat("Scale", &scale, 0.1f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::Checkbox("wireframe", &wireframe);
        ImGui::Text("Vertices: %llu", positions.size());
        for (int i = 0; i < positions.size(); i++) {
            ImGui::Text("Position %d: (%.2f, %.2f, %.2f)", i, positions[i].x, positions[i].y, positions[i].z);
        }
		ImGui::End();

        ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &cubeEBO);

    glfwTerminate();
    system.Stop();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, PhysicsSystem &system) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!CPressed) {
            firstMouse = true;
            mouseEnabled = !mouseEnabled;
            glfwSetInputMode(window, GLFW_CURSOR, (mouseEnabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED));
        }
        if (!mouseEnabled) {
            system.Play();
        } else {
            system.Pause();
        }
        CPressed = true;
    } else {
        CPressed = false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (mouseEnabled) {
        return;
    } else {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);
    
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }
    
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    
        lastX = xpos;
        lastY = ypos;
    
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

float min3(float a, float b, float c) {
    return std::min(a, std::min(b, c));
}

glm::vec3 hueToRGB(float hue) {
    float r, g, b, kr, kg, kb;

    kr = glm::mod((5 + hue * 6.0f), 6.0f);
    kg = glm::mod((3 + hue * 6.0f), 6.0f);
    kb = glm::mod((1 + hue * 6.0f), 6.0f);

    r = 1 - std::max(min3(kr, 4.0f-kr, 1.0f), 0.0f);
    g = 1 - std::max(min3(kg, 4.0f-kg, 1.0f), 0.0f);
    b = 1 - std::max(min3(kb, 4.0f-kb, 1.0f), 0.0f);

    return glm::vec3(r, g, b);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void printVec3(const glm::vec3& v) {
    std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
}