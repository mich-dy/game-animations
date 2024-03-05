#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "IForceGenerator.h"
#include "RigidBody.h"

class WindForce : public IForceGenerator {
  public:
    WindForce(const glm::vec3 wind);
    virtual ~WindForce() = default;

    void enable(const bool value);
    virtual void updateForce(std::shared_ptr<RigidBody> body, float deltaTime) override;

  private:
    glm::vec3 mWindAmount;
    bool mWindEnabled = false;
};
