#pragma once

#include <glm/glm.hpp>

class RigidBody {
  public:
    void integrate(const float deltaTime);

    void setPosition(const glm::vec3 pos);
    glm::vec3 getPosition() const;

    void setMass(const float mass);
    float getMass() const;
    bool hasInfiniteMass();
    float getInverseMass() const;

    void setVelocity(const glm::vec3 velo);
    glm::vec3 getVelocity() const;

    void setAcceleration(const glm::vec3 accel);
    glm::vec3 getAcceleration() const;

    void setDaming(const float damp);

    void addForce(const glm::vec3 force);
    void clearAccumulatedForce();

  private:
    // using the inverse of the mass is easier (i.e., inverse zero -> infinit mass)
    float mInverseMass = 0.0f;

    glm::vec3 mPosition = glm::vec3(0.0f);
    glm::vec3 mVelocity = glm::vec3(0.0f);
    glm::vec3 mAcceleration = glm::vec3(0.0f);
    float mDamping = 0.0;

    glm::vec3 mAccumulatedForce = glm::vec3(0.0f);
};
