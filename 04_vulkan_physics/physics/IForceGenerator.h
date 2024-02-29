#pragma once

#include "RigidBody.h"

/* Interface style base class */
class IForceGenerator {
  public:
    virtual void updateForce(RigidBody& body, float deltaTime) = 0;
};
