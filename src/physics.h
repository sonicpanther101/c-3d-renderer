#pragma once

#include "../vendor/glm/glm/glm.hpp"
#include "../vendor/glm/glm/gtc/matrix_transform.hpp"
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

class PhysicsSystem {
public:
    struct Particle {
        glm::vec3 position;
        glm::vec3 projection;
        glm::vec3 force = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
        float mass = 1.0f;
        float inverseMass = 1.0f / mass;
        float radius = 1.0f;
    };

    PhysicsSystem(std::vector<Particle> &particles);
    ~PhysicsSystem();
    
    void Start();
    void Stop();
    void Step();
    void GetParticlePositions(std::vector<glm::vec3>& positions);
    void GetParticleSizes(std::vector<float>& sizes);

private:
    std::chrono::high_resolution_clock::time_point m_LastTime;
    float m_dT;
    std::vector<Particle> m_PhysicsParticles;
    std::vector<Particle> m_RenderParticles;
    std::atomic<bool> m_Running{false};
    std::thread m_SimulationThread;
    std::mutex m_SwapMutex;
    
    static const float m_FIXED_DT;
    static const float m_DAMPING_CONSTANT;
    static const int m_SOLVER_ITERATIONS;

    void RunSimulationLoop();
    glm::vec3 externalForces(glm::vec3 *position);
    glm::mat3 skewSymmetric(const glm::vec3& r);
    void dampenVelocities();
    void generateCollisionConstraints(glm::vec3 *position, glm::vec3 *projection);
    void projectConstraints();
    void move();
};