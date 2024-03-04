#include "GravityForce.h"

#include "Logger.h"

GravityForce::GravityForce(const glm::vec3 gravity) : mGravityConstant(gravity) {};

void GravityForce::updateForce(std::shared_ptr<RigidBody> body, float deltaTime) {
  if (!body) {
    Logger::log(1, "%s error: no body given\n", __FUNCTION__);
    return;
  }

  if (body->hasInfiniteMass()) {
    return;
  }

  body->addForce(mGravityConstant * body->getMass());
}
