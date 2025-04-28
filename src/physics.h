#pragma once

#include "../vendor/glm/glm/glm.hpp"
#include "../vendor/glm/glm/gtc/matrix_transform.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

class PhysicsSystem {
public:
    struct Particle {
        glm::vec3 position;
        glm::vec3 lastPosition = position;
        glm::vec3 force = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
        float mass = 1.0f;
        float inverseMass = 1.0f / mass;
        float radius = 1.0f;
    };
    std::chrono::high_resolution_clock::time_point m_LastTime = std::chrono::high_resolution_clock::now();
    float m_dT;
    std::vector<Particle> m_Particles;
    std::vector<Particle> m_RenderParticles;
    std::atomic<bool> m_Running{false};
    std::thread m_SimulationThread;
    std::mutex m_ParticlesMutex;

    
    PhysicsSystem(std::vector<Particle> &particles) : m_Particles(particles) {}

    ~PhysicsSystem() {
        Stop();
    }
    
    void Start() {
        if (m_SimulationThread.joinable()) {
            return; // Avoid restarting if already running
        }
        m_Running = true;
        m_SimulationThread = std::thread(&PhysicsSystem::RunSimulationLoop, this);
    }
    
    void Stop() {
        m_Running = false;
        if (m_SimulationThread.joinable()) {
            m_SimulationThread.join();
        }
    }

    void Step() {
	    std::chrono::high_resolution_clock::time_point current = std::chrono::high_resolution_clock::now();
	    std::chrono::duration<float, std::milli> diff = current - m_LastTime;
        m_dT = diff.count();
        m_LastTime = current;
        // collisions();
        forces();
        move();

        std::lock_guard<std::mutex> lock(m_ParticlesMutex);
        m_RenderParticles = m_Particles;
    }

    void GetParticlePositions(std::vector<glm::vec3>& positions) {
        std::lock_guard<std::mutex> lock(m_ParticlesMutex);
        positions.clear();
        positions.reserve(m_RenderParticles.size());
        for (const auto& particle : m_RenderParticles) {
            positions.push_back(particle.position);
        }
    }

    void GetParticleSizes(std::vector<float>& sizes) {
        std::lock_guard<std::mutex> lock(m_ParticlesMutex);
        sizes.clear();
        sizes.reserve(m_RenderParticles.size());
        for (const auto& particle : m_RenderParticles) {
            sizes.push_back(particle.radius);
        }
    }
private:

    void RunSimulationLoop() {
        while (m_Running) {
            Step();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    void forces() {
        for (Particle &particle : m_Particles) {
            particle.force = glm::vec3(0.0f, 0.0f, 0.0f);
        }
    }

    void move() {
        for (Particle &particle : m_Particles) {
	        particle.velocity = particle.position - particle.lastPosition;
	        particle.lastPosition = particle.position;
	        particle.acceleration = particle.force * particle.inverseMass * m_dT * m_dT;
	        particle.position += particle.velocity + particle.acceleration;
        }
    }
};