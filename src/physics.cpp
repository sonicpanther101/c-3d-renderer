#include "physics.h"
#include <algorithm>
#include <iostream>
#include <cmath>

const float physicsFPS = 240.0f;
const float PhysicsSystem::m_FIXED_DT = 1.0f / physicsFPS;
const float PhysicsSystem::m_DAMPING_CONSTANT = 0.98f;

PhysicsSystem::PhysicsSystem(std::vector<Particle> &particles) 
    : m_PhysicsParticles(particles), m_RenderParticles(particles) {
    m_LastTime = std::chrono::high_resolution_clock::now();
}

PhysicsSystem::~PhysicsSystem() {
    Stop();
}

void PhysicsSystem::Start() {
    if (m_SimulationThread.joinable()) {
        return; // Avoid restarting if already running
    }
    m_Running = true;
    m_SimulationThread = std::thread(&PhysicsSystem::RunSimulationLoop, this);
}

void PhysicsSystem::Stop() {
    m_Running = false;
    if (m_SimulationThread.joinable()) {
        m_SimulationThread.join();
    }
}

void PhysicsSystem::Step() {
    std::chrono::high_resolution_clock::time_point current = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> diff = current - m_LastTime;
    m_dT = diff.count() / 1000.0f;
    if (m_dT > m_FIXED_DT) m_dT = m_FIXED_DT; // Clamp to avoid spiral of death
    m_LastTime = current;

    move();

    std::lock_guard<std::mutex> lock(m_SwapMutex);
    m_PhysicsParticles.swap(m_RenderParticles);
}

void PhysicsSystem::GetParticlePositions(std::vector<glm::vec3>& positions) {
    std::lock_guard<std::mutex> lock(m_SwapMutex);
    positions.clear();
    for (const auto& particle : m_RenderParticles) {
        positions.push_back(particle.position);
    }
}

void PhysicsSystem::GetParticleSizes(std::vector<float>& sizes) {
    std::lock_guard<std::mutex> lock(m_SwapMutex);
    sizes.clear();
    for (const auto& particle : m_RenderParticles) {
        sizes.push_back(particle.radius);
    }
}

void PhysicsSystem::RunSimulationLoop() {
    while (m_Running) {
        Step();
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(m_FIXED_DT * 1000)));
    }
}

glm::vec3 PhysicsSystem::externalForces(glm::vec3 *position) {
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::mat3 skewSymmetric(const glm::vec3& r) {
    return glm::mat3(
        0.0f,  -r.z,    r.y,
        r.z,    0.0f,  -r.x,
       -r.y,    r.x,    0.0f
    );
}

void PhysicsSystem::dampenVelocities(std::vector<Particle> *particles) {
    glm::vec3 CMPosition;
    glm::vec3 CMVelocity;

    glm::vec3 sumPosxMass;
    glm::vec3 sumVelxMass;
    glm::vec3 sumMass;
   
    for (Particle &particle : *particles) {
        sumPosxMass += particle.position * particle.mass;
        sumVelxMass += particle.velocity * particle.mass;
        sumMass += particle.mass;
    }

    CMPosition = sumPosxMass / sumMass;
    CMVelocity = sumVelxMass / sumMass;

    glm::vec3 angularMomentum;
    glm::mat3 inertiaTensor;
    
    for (Particle &particle : *particles) {
        
        glm::vec3 rad = particle.position - CMPosition;
        
        angularMomentum += glm::cross(rad, particle.velocity * particle.mass);
        
        glm::mat3 radTensor = skewSymmetric(rad);
        
        inertiaTensor += radTensor * glm::transpose(radTensor) * particle.mass;
    }

    glm::vec3 angularVelocity = glm::inverse(inertiaTensor) * angularMomentum;

    for (Particle &particle : *particles) {
        glm::vec3 deltaVelocity = CMVelocity + glm::cross(angularVelocity, particle.position - CMPosition) - particle.velocity;
        particle.velocity += m_DAMPING_CONSTANT * deltaVelocity;
    }
}

void PhysicsSystem::move() {
    // (5)
    for (Particle &particle : m_PhysicsParticles) {
        particle.velocity = particle.position + m_dT * particle.inverseMass * externalForces(&particle.position);
    }
    // (6)
    dampenVelocities(&m_PhysicsParticles);

}