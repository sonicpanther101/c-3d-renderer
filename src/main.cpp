#include <GL/glew.h>
#include <GLFW/glfw3.h>

// GLM
#include "../vendor/glm/glm/gtc/type_ptr.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <cstdio>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "shader.h"
#include "camera.h"
#include "physics.h"
#include "model.h"

#include <atomic>
#include <iostream>
#include <vector>

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
bool EscPressed = false;
bool wireframe = false;
bool points = false;
bool triangles = true;
bool floorEnabled = true;

// camera
Camera camera(glm::vec3(0.0f, 5.0f, 15.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

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

struct Vertex1 {
    glm::vec3 position;
    glm::vec3 normal;
};

// physics vertex dragging
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// Global state for dragging
bool isDragging = false;
int selectedVertex = -1;
glm::vec3 originalVertexPosition;
glm::vec3 dragPlaneNormal;
float savedInverseMass = 0.0f;

glm::vec3 getRayFromMouse(double x, double y, const glm::mat4& projection, const glm::mat4& view) {
    float ndcX = (2.0f * (float)x) / SCR_WIDTH - 1.0f;
    float ndcY = 1.0f - (2.0f * (float)y) / SCR_HEIGHT;

    glm::vec4 clipCoords(ndcX, ndcY, -1.0f, 1.0f);
    glm::mat4 invProj = glm::inverse(projection);
    glm::vec4 eyeCoords = invProj * clipCoords;
    eyeCoords = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0f, 0.0f);

    glm::mat4 invView = glm::inverse(view);
    glm::vec4 worldCoords = invView * eyeCoords;

    return glm::normalize(glm::vec3(worldCoords));
}

int findClosestVertex(const std::vector<glm::vec3>& positions, const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDistance) {
    int closestVertex = -1;
    float minDistance = maxDistance;
    
    for (int i = 0; i < positions.size(); i++) {
        glm::vec3 diff = positions[i] - rayOrigin;
        float t = glm::dot(diff, rayDir);
        
        if (t < 0) continue;
        
        glm::vec3 projection = rayOrigin + t * rayDir;
        float distance = glm::distance(positions[i], projection);
        
        if (distance < minDistance) {
            minDistance = distance;
            closestVertex = i;
        }
    }
    return closestVertex;
}

static std::atomic<float> targetVolume(0.0f);


int main() {

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
    glfwSetMouseButtonCallback(window, mouse_button_callback);
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

    std::vector<PhysicsSystem::Particle> vertices;
    std::vector<PhysicsSystem::Constraint> edges;

    // load models
    // -----------
    Model Bunny("../resources/objects/bunny.obj");

    float modelScale = 50.0f;

    // Calculate volume of mesh
    float initialVolume = 0.0f;
    std::vector<std::vector<unsigned int>> Triangles;
    for (int i = 0; i < Bunny.meshes.size(); i++) {
        Mesh& mesh = Bunny.meshes[i];
        for (int j = 0; j < mesh.indices.size(); j += 3) {
            unsigned int idx0 = mesh.indices[j];
            unsigned int idx1 = mesh.indices[j+1];
            unsigned int idx2 = mesh.indices[j+2];
            
            glm::vec3 v0 = mesh.vertices[idx0].Position * modelScale;
            glm::vec3 v1 = mesh.vertices[idx1].Position * modelScale;
            glm::vec3 v2 = mesh.vertices[idx2].Position * modelScale;
            
            initialVolume += glm::dot(v0, glm::cross(v1, v2));
            Triangles.push_back({idx0, idx1, idx2});
        }
    }
    initialVolume /= 6.0f;
    targetVolume.store(initialVolume);

    for (int i = 0; i < Bunny.meshes.size(); i++) {
        for (int j = 0; j < Bunny.meshes[i].vertices.size(); j++) {
            Bunny.meshes[i].vertices[j].Position *= modelScale;
        }
    }

    // get number of vertices
    int nvertices = 0;
    for (int i = 0; i < Bunny.meshes.size(); i++) {
        nvertices += Bunny.meshes[i].vertices.size();
    }

    float mass = 1.0f / nvertices;

    // convert to vertices
    for (int i = 0; i < Bunny.meshes.size(); i++) {
        for (int j = 0; j < Bunny.meshes[i].vertices.size(); j++) {
            vertices.push_back(PhysicsSystem::Particle(
                vertices.size(),  // Generate unique index
                Bunny.meshes[i].vertices[j].Position,
                glm::vec3(0.0f),
                mass
            ));
        }
    }
    for (int i = 0; i < vertices.size(); i++) {
        vertices[i].velocity = glm::vec3(0.0f, 0.0f, 50.0f);
    }

    // convert to edges
    for (int i = 0; i < Bunny.meshes.size(); i++) {
        Mesh& mesh = Bunny.meshes[i];
        for (int j = 0; j < mesh.indices.size(); j+=3) {
            unsigned int idx[3] = {
                mesh.indices[j],
                mesh.indices[j+1],
                mesh.indices[j+2]
            };
            for (int k = 0; k < 3; k++) {
                // very clever ai indices
                unsigned int p1 = idx[k];
                unsigned int p2 = idx[(k+1)%3];

                glm::vec3 pos1 = mesh.vertices[p1].Position;
                glm::vec3 pos2 = mesh.vertices[p2].Position;
                float originalDistanceBetween = glm::length(pos1 - pos2);

                edges.push_back(PhysicsSystem::Constraint(
                    {p1, p2}, 
                    0.7f,              // stiffness
                    true,              // equality
                    [originalDistanceBetween](std::vector<PhysicsSystem::Particle*> particles) -> float {
                        return glm::length(particles[0]->position - particles[1]->position) - originalDistanceBetween;
                    },
                    [](std::vector<PhysicsSystem::Particle*> particles) -> std::vector<glm::vec3> {
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
        }
    }
    
    // other parts needed for rendering
    unsigned int numVertices = vertices.size();
    
    std::vector<unsigned int> faceIndices;
    for (int i = 0; i < Bunny.meshes.size(); i++) {
        const auto& indices = Bunny.meshes[i].indices;
        faceIndices.insert(faceIndices.end(), indices.begin(), indices.end());
    }
    
    std::vector<unsigned int> edgeIndices;
    for (const auto& constraint : edges) {
        edgeIndices.push_back(constraint.indices[0]);
        edgeIndices.push_back(constraint.indices[1]);
    }

    // Volume constraints
    std::vector<unsigned int> allIndices;
    for (int i = 0; i < vertices.size(); i++) {
        allIndices.push_back(i);
    }
    
    edges.push_back(PhysicsSystem::Constraint(
        allIndices,
        0.9f,              // stiffness
        true,               // equality
        [Triangles](std::vector<PhysicsSystem::Particle*> particles) -> float {
            float currentVolume = 0.0f;
            for (const auto& tri : Triangles) {
                glm::vec3 p0 = particles[tri[0]]->position;
                glm::vec3 p1 = particles[tri[1]]->position;
                glm::vec3 p2 = particles[tri[2]]->position;
                currentVolume += glm::dot(p0, glm::cross(p1, p2));
            }
            currentVolume /= 6.0f;
            return currentVolume - targetVolume.load();
        },
        [Triangles](std::vector<PhysicsSystem::Particle*> particles) -> std::vector<glm::vec3> {
            std::vector<glm::vec3> gradients(particles.size(), glm::vec3(0.0f));
            for (const auto& tri : Triangles) {
                glm::vec3 p0 = particles[tri[0]]->position;
                glm::vec3 p1 = particles[tri[1]]->position;
                glm::vec3 p2 = particles[tri[2]]->position;
                
                gradients[tri[0]] += (1.0f/6.0f) * glm::cross(p1, p2);
                gradients[tri[1]] += (1.0f/6.0f) * glm::cross(p2, p0);
                gradients[tri[2]] += (1.0f/6.0f) * glm::cross(p0, p1);
            }
            return gradients;
        }
    ));
    
    PhysicsSystem physicsSystem(vertices, edges);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // render 1 frame to stop flashbang startup
    processInput(window, physicsSystem);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwPollEvents();

    glfwSetWindowUserPointer(window, &physicsSystem);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int billboardVAO, billboardVBO, pointsVBO, indexVBO;
    const float quadVertices[] = {
        // Positions
        -0.5f,  0.5f,
        -0.5f, -0.5f,
        0.5f, -0.5f,
        -0.5f,  0.5f,
        0.5f, -0.5f,
        0.5f,  0.5f
    };

    // build and compile our shader program
    // ------------------------------------
    Shader pointShader("../shaders/point_vertex.glsl", "../shaders/point_fragment.glsl");
    Shader lineShader("../shaders/line_vertex.glsl", "../shaders/line_fragment.glsl");
    Shader cubeShader("../shaders/cube_vertex.glsl", "../shaders/cube_fragment.glsl");
    Shader floorShader("../shaders/floor_vertex.glsl", "../shaders/floor_vertex.glsl");

    pointShader.use();

    pointShader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));

    cubeShader.use();
    cubeShader.setVec3("lightPos", glm::vec3(5.0f, 5.0f, 5.0f));
    cubeShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    cubeShader.setVec3("objectColor", glm::vec3(0.0f, 0.0f, 1.0f));

    floorShader.use();

    // world transformation
    glm::mat4 model = glm::mat4(1.0f);
    cubeShader.setMat4("model", model);
    floorShader.setMat4("model", model);

    glGenVertexArrays(1, &billboardVAO);
    glGenBuffers(1, &billboardVBO);
    glGenBuffers(1, &pointsVBO);
    glGenBuffers(1, &indexVBO);

    glBindVertexArray(billboardVAO);

    // Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, billboardVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // Instance buffer
    glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
    glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(1, 1);  // Update once per instance

    glBindVertexArray(0);

    // rendering floor
    std::vector<glm::vec3> floor = {
        glm::vec3(-0.5f, 0.0f, -0.5f),
        glm::vec3( 0.0f, 1.0f,  0.0f),
        glm::vec3( 0.5f, 0.0f, -0.5f),
        glm::vec3( 0.0f, 1.0f,  0.0f),
        glm::vec3(-0.5f, 0.0f,  0.5f),
        glm::vec3( 0.0f, 1.0f,  0.0f),

        glm::vec3( 0.5f, 0.0f, -0.5f),
        glm::vec3( 0.0f, 1.0f,  0.0f),
        glm::vec3(-0.5f, 0.0f,  0.5f),
        glm::vec3( 0.0f, 1.0f,  0.0f),
        glm::vec3( 0.5f, 0.0f,  0.5f),
        glm::vec3( 0.0f, 1.0f,  0.0f)
    };

    float floorScale = 20.0f;
    for (int i = 0; i < floor.size(); i+=2)
        floor[i] *= floorScale;

    unsigned int floorVAO, floorVBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);

    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, floor.size() * sizeof(glm::vec3), floor.data(), GL_DYNAMIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2*sizeof(glm::vec3), (void*)0);

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2*sizeof(glm::vec3), (void*)sizeof(glm::vec3));

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
    glBufferData(GL_ARRAY_BUFFER, faceIndices.size() * sizeof(Vertex1), nullptr, GL_DYNAMIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex1), (void*)0);

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex1), (void*)offsetof(Vertex1, normal));

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
    ImGui_ImplOpenGL3_Init("#version 450 core");

	physicsSystem.Start();
	
    // render loop
    // -----------

    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        std::vector<glm::vec3> positions;
        physicsSystem.GetParticlePositions(positions);

        std::vector<float> sizes;
        physicsSystem.GetParticleSizes(sizes);

        // Update instance data
        glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(glm::vec3), positions.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Update cube positions

        // Generate expanded vertices with normals
        std::vector<Vertex1> cubeVertices;
        cubeVertices.reserve(faceIndices.size() * 3);

        for (int i = 0; i < faceIndices.size(); i += 3) {
            // Get triangle indices
            unsigned int idx0 = faceIndices[i];
            unsigned int idx1 = faceIndices[i+1];
            unsigned int idx2 = faceIndices[i+2];
            
            // Get positions
            glm::vec3 v0 = positions[idx0];
            glm::vec3 v1 = positions[idx1];
            glm::vec3 v2 = positions[idx2];
            
            // Calculate face normal
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
            
            // Add vertices with normals
            cubeVertices.push_back({v0, normal});
            cubeVertices.push_back({v1, normal});
            cubeVertices.push_back({v2, normal});
        }

        // Update VBO
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, cubeVertices.size() * sizeof(Vertex1), cubeVertices.data());

        // Update cube line positions
        glBindBuffer(GL_ARRAY_BUFFER, cubelinesVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(glm::vec3), positions.data());


        // input
        // -----
        processInput(window, physicsSystem);

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

        if (triangles) {
            // Solid triangles
            cubeShader.use();
            cubeShader.setMat4("projection", projection);
            cubeShader.setMat4("view", view);
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());
        }

        if (wireframe) {
            // Lines
            glLineWidth(4.0f); // Make lines thicker
            lineShader.use();
            lineShader.setMat4("projection", projection);
            lineShader.setMat4("view", view);
            glBindVertexArray(cubelinesVAO);
            glDrawElements(GL_LINES, edgeIndices.size(), GL_UNSIGNED_INT, 0);
        }

        if (points) {
            // be sure to activate shader when setting uniforms/drawing objects
            pointShader.use();

            pointShader.setVec3("viewPos", camera.Position);
            pointShader.setFloat("time", static_cast<float>(glfwGetTime()));

            // Billboard-specific uniforms
            pointShader.setVec3("cameraRight", camera.Right);
            pointShader.setVec3("cameraUp", camera.Up);

            // Draw billboards
            glBindVertexArray(billboardVAO);
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, positions.size());
        }

        if (floorEnabled) {
            floorShader.use();
            floorShader.setMat4("projection", projection);
            floorShader.setMat4("view", view);

            glBindVertexArray(floorVAO);
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, floor.size()/2);
        }

        // ImGui
        ImGui::Begin("Changer");
        ImGui::Checkbox("wireframe", &wireframe);
        ImGui::Checkbox("points", &points);
        ImGui::Checkbox("triangles", &triangles);
        ImGui::Checkbox("floor", &floorEnabled);
        ImGui::Text("Vertices: %llu", positions.size());
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);

        // Volume control slider
        float vol = targetVolume.load();
        if (ImGui::SliderFloat("Target Volume", &vol, 0.0f, initialVolume*10.0f)) {
            targetVolume.store(vol);
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
    physicsSystem.Stop();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, PhysicsSystem &physicsSystem) {
    if (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS)
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
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (!EscPressed) {
            firstMouse = true;
            mouseEnabled = !mouseEnabled;
            glfwSetInputMode(window, GLFW_CURSOR, (mouseEnabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED));
        }
        if (!mouseEnabled) {
            physicsSystem.Play();
        } else {
            physicsSystem.Pause();
        }
        EscPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!CPressed) {
            firstMouse = true;
            mouseEnabled = !mouseEnabled;
            glfwSetInputMode(window, GLFW_CURSOR, (mouseEnabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED));
        }
        CPressed = true;
    } else {
        CPressed = false;
        EscPressed = false;
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

    PhysicsSystem* physicsSystemPtr = static_cast<PhysicsSystem*>(glfwGetWindowUserPointer(window));
    if (!physicsSystemPtr) return;
    PhysicsSystem& physicsSystem = *physicsSystemPtr;

    if (isDragging) {
        double x = xposIn;
        double y = yposIn;
        
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 
            (float)SCR_WIDTH/(float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::vec3 rayDir = getRayFromMouse(x, y, projection, view);

        float denom = glm::dot(rayDir, dragPlaneNormal);
        if (fabs(denom) > 1e-6) {
            float t = glm::dot(originalVertexPosition - camera.Position, dragPlaneNormal) / denom;
            if (t > 0) {
                glm::vec3 newPosition = camera.Position + t * rayDir;

                std::lock_guard<std::mutex> lock(physicsSystem.m_SwapMutex);
                PhysicsSystem::Particle& p = physicsSystem.m_PhysicsParticles[selectedVertex];
                PhysicsSystem::Particle& r = physicsSystem.m_RenderParticles[selectedVertex];
                
                p.position = newPosition;
                r.position = newPosition;
                p.projection = newPosition;
                r.projection = newPosition;
                p.velocity = glm::vec3(0.0f);
                r.velocity = glm::vec3(0.0f);
            }
        }
    } else {
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
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

    PhysicsSystem* physicsSystemPtr = static_cast<PhysicsSystem*>(glfwGetWindowUserPointer(window));
    if (!physicsSystemPtr) return;
    PhysicsSystem& physicsSystem = *physicsSystemPtr;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 
            (float)SCR_WIDTH/(float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::vec3 rayDir = getRayFromMouse(x, y, projection, view);
        
        std::vector<glm::vec3> positions;
        physicsSystem.GetParticlePositions(positions);
        
        selectedVertex = findClosestVertex(positions, camera.Position, rayDir, 0.5f);
        
        if (selectedVertex != -1) {
            isDragging = true;
            originalVertexPosition = positions[selectedVertex];
            dragPlaneNormal = camera.Up;
            
            std::lock_guard<std::mutex> lock(physicsSystem.m_SwapMutex);
            PhysicsSystem::Particle& p = physicsSystem.m_PhysicsParticles[selectedVertex];
            PhysicsSystem::Particle& r = physicsSystem.m_RenderParticles[selectedVertex];
            
            savedInverseMass = p.inverseMass;
            p.inverseMass = 0.0f;
            r.inverseMass = 0.0f;
            p.velocity = glm::vec3(0.0f);
            r.velocity = glm::vec3(0.0f);
        }
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        if (isDragging) {
            std::lock_guard<std::mutex> lock(physicsSystem.m_SwapMutex);
            PhysicsSystem::Particle& p = physicsSystem.m_PhysicsParticles[selectedVertex];
            PhysicsSystem::Particle& r = physicsSystem.m_RenderParticles[selectedVertex];
            
            p.inverseMass = savedInverseMass;
            r.inverseMass = savedInverseMass;
            p.velocity = glm::vec3(0.0f);
            r.velocity = glm::vec3(0.0f);
            
            isDragging = false;
            selectedVertex = -1;
        }
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