#include "WindForce.h"

#include "Logger.h"

WindForce::WindForce(const glm::vec3 wind) : mWindAmount(wind) {};

void WindForce::enable(const bool value) {
  mWindEnabled = value;
}

void WindForce::updateForce(std::shared_ptr<RigidBody> body, float deltaTime) {
  if (!body) {
    Logger::log(1, "%s error: no body given\n", __FUNCTION__);
    return;
  }

  if (body->hasInfiniteMass()) {
    return;
  }

  if (mWindEnabled) {
    body->addForce(mWindAmount * body->getMass());
    body->addForceToBodyPoint(mWindAmount, body->getPosition() - glm::vec3(0.0f, 0.5f, 0.0f));
  }
}
