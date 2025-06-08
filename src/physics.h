#pragma once

#include "../vendor/glm/glm/glm.hpp"
#include "../vendor/glm/glm/gtc/matrix_transform.hpp"
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>

const float EPSILON = 1e-6f;

class PhysicsSystem {
public:
    struct Particle {
        int index;
        glm::vec3 position;
        glm::vec3 projection;
        glm::vec3 force = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 velocity;
        glm::vec3 acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
        float mass;
        float inverseMass;
        float radius;

        Particle(int Index, glm::vec3 Position, glm::vec3 Velocity, float Mass = 1.0f, float Radius = 1.0f) {
            index = Index;
            position = Position;
            projection = Position;
            velocity = Velocity;
            mass = Mass;
            inverseMass = 1.0f / mass;
            radius = Radius;
        }
    };

    struct Constraint {
        std::vector<unsigned int> indices;
        float stiffness;
        float kPrime;
        bool equality;
        int cardinality;

        std::function<float(std::vector<Particle*>)> function;
        std::function<std::vector<glm::vec3>(std::vector<Particle*>)> gradient;

        static float defaultFunction(std::vector<Particle*> particles) {
            if (particles.size() != 2) {
                std::cout << "Constraint for that dimension is not supported yet" << std::endl;
                return 0.0f;
            }
            glm::vec3 difference = particles[0]->position - particles[1]->position;
            return glm::length(difference) - 1.0f;
        }

        static std::vector<glm::vec3> defaultGradient(std::vector<Particle*> particles) {
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

        Constraint(
            std::vector<unsigned int> Indices, 
            float Stiffness = 0.98f, 
            bool Equality = true, 
            std::function<float(std::vector<Particle*>)> Function = defaultFunction,
            std::function<std::vector<glm::vec3>(std::vector<Particle*>)> Gradient = defaultGradient
        ) : 
            indices(Indices), 
            stiffness(Stiffness),
            equality(Equality), 
            function(Function),
            gradient(Gradient)
        {
            cardinality = Indices.size();
            kPrime = 1.0f - stiffness;
        }
    };

    PhysicsSystem(std::vector<Particle> &particles, std::vector<Constraint> &constraints);
    ~PhysicsSystem();
    
    void Start();
    void Stop();
    void Pause();
    void Play();
    void Step();
    void GetParticlePositions(std::vector<glm::vec3>& positions);
    void GetParticleSizes(std::vector<float>& sizes);

private:
    std::chrono::high_resolution_clock::time_point m_LastTime;
    float m_dT;
    std::vector<Particle> m_PhysicsParticles;
    std::vector<Particle> m_RenderParticles;
    std::vector<Constraint> m_Constraints;
    std::atomic<bool> m_Running{false};
    std::atomic<bool> m_Paused{false};
    std::thread m_SimulationThread;
    std::mutex m_SwapMutex;
    
    static const float m_FIXED_DT;
    static const float m_DAMPING_CONSTANT;
    static const int m_SOLVER_ITERATIONS;

    void RunSimulationLoop();
    glm::vec3 externalForces(glm::vec3 *position);
    glm::mat3 skewSymmetric(const glm::vec3& r);
    void dampenVelocities();
    void generateCollisionConstraints(Particle particle);
    void projectConstraints();
    void move();
};