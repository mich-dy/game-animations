#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "IForceGenerator.h"
#include "RigidBody.h"

class GravityForce : public IForceGenerator {
  public:
    GravityForce(const glm::vec3 gravity);
    virtual ~GravityForce() = default;

    virtual void updateForce(std::shared_ptr<RigidBody> body, float deltaTime) override;

  private:
    glm::vec3 mGravityConstant;
};
