#pragma once

#include <memory>

#include "RigidBody.h"

/* Interface style base class */
class IForceGenerator {
  public:
    virtual void updateForce(std::shared_ptr<RigidBody> body, float deltaTime) = 0;
};
