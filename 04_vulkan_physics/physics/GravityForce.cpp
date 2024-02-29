#include "GravityForce.h"

#include "Logger.h"

GravityForce::GravityForce(const glm::vec3 gravity) : mGravityConstant(gravity) {};

void GravityForce::updateForce(std::shared_ptr<RigidBody> body, float deltaTime) {
  if (body && body->hasInfiniteMass()) {
    return;
  }

  body->addForce(mGravityConstant * body->getMass());
}
