#pragma once

#include "../vendor/glm/glm/glm.hpp"
#include "../vendor/glm/glm/gtc/matrix_transform.hpp"
#include <range/v3/all.hpp>
#include <vector>
#include <algorithm>
#include <iostream>

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
    float m_dT;
    std::vector<Particle> m_Particles;
    
    PhysicsSystem(std::vector<Particle> &particles) {
        m_Particles = particles;
    }

    void Step(float &dT) {
        m_dT = dT;
        // collisions()
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