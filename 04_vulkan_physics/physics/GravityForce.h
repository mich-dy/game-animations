#pragma once
#include <glm/glm.hpp>

#include "IForceGenerator.h"
#include "RigidBody.h"

class GravityForce : public IForceGenerator {
  public:
    GravityForce(const glm::vec3 gravity);

    virtual void updateForce(RigidBody& body, float deltaTime) override;

  private:
    glm::vec3 mGravityConstant;
};
