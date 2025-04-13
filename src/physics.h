#pragma once

#include "../vendor/glm/glm/glm.hpp"
#include "../vendor/glm/glm/gtc/matrix_transform.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include <chrono>

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
    bool m_Running = true;
    
    PhysicsSystem(std::vector<Particle> &particles) {
        m_Particles = particles;
    }
    
    void Start() {
	    while (m_Running) {
		    Step();
	    }
    }
    
    void Stop() {
	    m_Running = false;
    }

    void Step() {
	    std::chrono::high_resolution_clock::time_point current = std::chrono::high_resolution_clock::now();
	    std::chrono::duration<float, std::milli> diff = current - m_LastTime;
        m_dT = diff.count();
        m_LastTime = current;
        // collisions();
        forces();
        move();
    }
private:

    void forces() {
        for (Particle &particle : m_Particles) {
            particle.force = glm::vec3(0.0f, 0.0f, 0.0f);
        }
    }

    void move() {
        for (Particle &particle : m_Particles) {
	        particle.velocity = particle.position - lastPosition;
	        particle.lastPosition = particle.position;
	        particle.acceleration = particle.force * particle.inverseMass * m_dT * m_dT;
	        particle.position += particle.velocity + particle.acceleration;
        }
    }
}