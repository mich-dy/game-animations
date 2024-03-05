#include "DragForce.h"
#include "Logger.h"

DragForce::DragForce(const float linearCoeff, const float squareCoeff): mLinearDragCoefficient(linearCoeff), mSquareDragCoefficient(squareCoeff) { }

void DragForce::updateForce(std::shared_ptr<RigidBody> body, float deltaTime) {
  if (!body) {
    Logger::log(1, "%s error: no body given\n", __FUNCTION__);
    return;
  }

  if (body->hasInfiniteMass()) {
    return;
  }

  glm::vec3 velocity = body->getVelocity();
  float velocityLength = glm::length(velocity);

  float dragCoeff = mLinearDragCoefficient * velocityLength + mSquareDragCoefficient * velocityLength * velocityLength;
  glm::vec3 force = glm::normalize(velocity) * -dragCoeff;

  if (glm::any(glm::isnan(force))) {
    Logger::log(1, "%s error: springVector contains a NaN value\n", __FUNCTION__);
    return;
  }

  body->addForce(force * body->getMass());
}

