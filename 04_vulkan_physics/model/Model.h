#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "VkRenderData.h"

class Model {
  public:
    VkMesh getVertexData();

    glm::vec3 getPosition();

    void setMass(const float mass);
    void setPosition(const glm::vec3 pos);
    void setVelocity(const glm::vec3 velo);
    void setAcceleration(const glm::vec3 acc);
    void setDaming(const float damp);

    void addForce(const glm::vec3 force);

    void setPhysicsEnabled(const bool value);
    void updatePhysics(const float deltaTime);

  private:
    void init();
    VkMesh mVertexData;


    bool mPhysicsEnabled;

    glm::vec3 mPosition;
    glm::vec3 mVelocity;
    glm::vec3 mAcceleration;
    double mDamping;

    // using the inverse of the mass is easier (i.e., inverse zero -> infinit mass)
    double mInverseMass;

    void clearAccumulatedForce();
    glm::vec3 mAccumulatedForce;
};
