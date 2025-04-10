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

    std::vector<glm::vec3> PP;
    std::vector<glm::vec3> LPP;
    std::vector<glm::vec3> Forces;
    std::vector<float> PS;
    std::vector<float> PM;
    float dT;

    PhysicsSystem(std::vector<glm::vec3> &particalPositions, std::vector<glm::vec3> &lastParticalPositions, std::vector<float> &particalMasses, std::vector<float> &particalSizes) {
        if (particalPositions.size() != lastParticalPositions.size() || particalPositions.size() != particalSizes.size() || particalPositions.size() != particalMasses.size())
            std::cout << "they don't match buddy" << std::endl;
        this->PP  = particalPositions;
        this->LPP = lastParticalPositions;
        this->PM  = particalMasses;
        this->PS  = particalSizes;
        this->Forces = std::vector<glm::vec3>(PP.size(), glm::vec3(0.0f));
    }

    void Step(float &dT) {
        this->dT = dT;
        // collisions()
        forces();
        move();
    }
private:

    void forces() {
        this->Forces = std::vector<glm::vec3>(PP.size(), glm::vec3(0.0f));
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