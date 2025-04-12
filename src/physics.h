#ifndef PHYSICS_H
#define PHYSICS_H

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
        float mass = 1.0f;
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
        std::vector<glm::vec3> velocity(this->PP.size());

        std::transform(
            this->PP.begin(), this->PP.end(),
            this->LPP.begin(),
            velocity.begin(),
            [](const glm::vec3 &pp, const glm::vec3 &lpp) {
                return pp - lpp; // Subtract element-wise
            }
        );

        this->LPP = this->PP;

        std::vector<glm::vec3> acceleration(this->PP.size());

        std::transform(
            this->Forces.begin(), this->Forces.end(),
            this->PM.begin(),
            acceleration.begin(),
            [this](const glm::vec3 &pf, const float &pm) {
                return (pf / pm) * (this->dT * this->dT);
            }
        );

        auto zip = ranges::views::zip(this->PP, velocity, acceleration);
        std::transform(
            zip.begin(), zip.end(),
            std::back_inserter(this->PP),
            [](const auto &tup) {
                const auto &pos = std::get<0>(tup);
                const auto &vel = std::get<1>(tup);
                const auto &acc = std::get<2>(tup);
                return pos + vel + acc;
            }
        );
    }
};

#endif