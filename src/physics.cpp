#include "physics.h"
#include <algorithm>
#include <iostream>
#include <cmath>

const float physicsFPS = 240.0f;
const float PhysicsSystem::m_FIXED_DT = 1.0f / physicsFPS;
const float PhysicsSystem::m_DAMPING_CONSTANT = 0.98f;
const int PhysicsSystem::m_SOLVER_ITERATIONS = 10;

PhysicsSystem::PhysicsSystem(std::vector<Particle> &particles, std::vector<Constraint> &constraints) 
    : m_PhysicsParticles(particles), m_RenderParticles(particles), m_Constraints(constraints) {
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

void PhysicsSystem::Pause() {
    m_Paused.store(true);
}

void PhysicsSystem::Play() {
    m_Paused.store(false);
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
        if (!m_Paused.load()) {  // Check pause state
            Step();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(m_FIXED_DT * 1000)));
    }
}

glm::vec3 PhysicsSystem::externalForces(glm::vec3 *position) {
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::mat3 PhysicsSystem::skewSymmetric(const glm::vec3& r) {
    return glm::mat3(
        0.0f,  -r.z,    r.y,
        r.z,    0.0f,  -r.x,
       -r.y,    r.x,    0.0f
    );
}

void PhysicsSystem::dampenVelocities() {
    // (1) & (2)
    glm::vec3 CMPosition;
    glm::vec3 CMVelocity;

    glm::vec3 sumPosxMass = glm::vec3(0.0f);
    glm::vec3 sumVelxMass = glm::vec3(0.0f);
    glm::vec3 sumMass = glm::vec3(0.0f);
   
    for (Particle &particle : m_PhysicsParticles) {
        sumPosxMass += particle.position * particle.mass;
        sumVelxMass += particle.velocity * particle.mass;
        sumMass += particle.mass;
    }

    // (3) & (4)

    CMPosition = sumPosxMass / sumMass;
    CMVelocity = sumVelxMass / sumMass;

    glm::vec3 angularMomentum = glm::vec3(0.0f);
    glm::mat3 inertiaTensor = glm::mat3(0.0f);
    
    for (Particle &particle : m_PhysicsParticles) {
        
        glm::vec3 rad = particle.position - CMPosition;
        
        angularMomentum += glm::cross(rad, particle.velocity * particle.mass);
        
        glm::mat3 radTensor = skewSymmetric(rad);
        
        inertiaTensor += radTensor * glm::transpose(radTensor) * particle.mass;
    }

    // (5)

    glm::vec3 angularVelocity = glm::inverse(inertiaTensor) * angularMomentum;

    // (6)

    for (Particle &particle : m_PhysicsParticles) {
        glm::vec3 deltaVelocity = CMVelocity + glm::cross(angularVelocity, particle.position - CMPosition) - particle.velocity;
        particle.velocity += m_DAMPING_CONSTANT * deltaVelocity;
    }
}

void PhysicsSystem::generateCollisionConstraints(glm::vec3* position, glm::vec3* projection) {
    // TODO: Implement collision constraint generation
}

void PhysicsSystem::projectConstraints() {

    // eq (10) & (11) delta projection = w1/(w1+w2) * constraintDelta * (p1-p2)/|p1-p2|

    for (Constraint &constraint : m_Constraints) {

        
        Particle* particle1 = &m_PhysicsParticles[constraint.indecies[0]];
        Particle* particle2 = &m_PhysicsParticles[constraint.indecies[1]];
        
        glm::vec3 difference = particle1->position - particle2->position;
        
        float constraintDelta = constraint.distanceFunction(difference);
        
        if (constraint.equality && constraintDelta != 0.0f) return;
        if (!constraint.equality && constraintDelta >= 0.0f) return;

        glm::vec3 correction = constraintDelta * glm::normalize(difference) / (particle1->inverseMass + particle2->inverseMass);

        particle1->projection -= particle1->inverseMass * correction;
        particle2->projection += particle2->inverseMass * correction;
    }
}

void PhysicsSystem::move() {
    // (5)
    for (Particle &particle : m_PhysicsParticles) {
        particle.velocity += m_dT * particle.inverseMass * externalForces(&particle.position);
    }

    // (6)
    dampenVelocities();

    // (7)
    for (Particle &particle : m_PhysicsParticles) {
        particle.projection = particle.position + m_dT * particle.velocity;
    }

    // (8)
    for (Particle &particle : m_PhysicsParticles) {
        generateCollisionConstraints(&particle.position, &particle.projection);
    }

    // (9) - (11)
    for (uint16_t i = 0; i < m_SOLVER_ITERATIONS; i++) {
        // (10)
        projectConstraints();
    }

    // (12) - (15)
    for (Particle &particle : m_PhysicsParticles) {
        // (13)
        particle.velocity = (particle.projection - particle.position) / m_dT;
        // (14)
        particle.position = particle.projection;
    }

    // (16)
    // dampen velocities of vertices involved in a collision perpendicular to the collision normal and reflected in the direction of the collision normal
}