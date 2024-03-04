#pragma once

#include <glm/glm.hpp>

#include "IForceGenerator.h"

class AnchoredSpringForce : public IForceGenerator {
  public:
    AnchoredSpringForce(const glm::vec3 anchor, const float springConstant, const float restLength);
    virtual ~AnchoredSpringForce() = default;

    virtual void updateForce(std::shared_ptr<RigidBody> body, float deltaTime) override;

  private:
    glm::vec3 mSpringAnchor = glm::vec3(0.0f);
    float mSpringConstant = 0.0f;
    float mSpringRestLength = 0.0f;
};
