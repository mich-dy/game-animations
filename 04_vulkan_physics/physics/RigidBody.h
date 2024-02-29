#pragma once

#include <glm/glm.hpp>

class RigidBody {
  public:
    void updatePhysics(const float deltaTime);

    void setPosition(const glm::vec3 pos);
    glm::vec3 getPosition() const;

    void setMass(const float mass);
    float getMass() const;
    bool hasInfiniteMass();

    void setVelocity(const glm::vec3 velo);
    void setAcceleration(const glm::vec3 accel);
    void setDaming(const float damp);

    void addForce(const glm::vec3 force);
    void clearAccumulatedForce();


  private:
    // using the inverse of the mass is easier (i.e., inverse zero -> infinit mass)
    double mInverseMass;

    glm::vec3 mPosition;
    glm::vec3 mVelocity;
    glm::vec3 mAcceleration;
    double mDamping;

    glm::vec3 mAccumulatedForce;
};