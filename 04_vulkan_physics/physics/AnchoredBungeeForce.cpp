#include "AnchoredBungeeForce.h"

#include "Logger.h"

AnchoredBungeeForce::AnchoredBungeeForce(const glm::vec3 anchor, const float springConstant, const float restLength) :
    mSpringAnchor(anchor), mSpringConstant(springConstant), mSpringRestLength(restLength) {
}

void AnchoredBungeeForce::updateForce(std::shared_ptr<RigidBody> body, float deltaTime) {
  if (!body) {
    Logger::log(1, "%s error: no body given\n", __FUNCTION__);
    return;
  }

  if (body->hasInfiniteMass()) {
    return;
  }

  /* calculate vector of spring between anchor and body position  */
  glm::vec3 springVector = body->getPosition() - mSpringAnchor;
  float springVectorLength = glm::length(springVector);

  /* rubber band: no force when compressed beyound rest length  */
  if (springVectorLength <= mSpringRestLength) {
    return;
  }

  /* calculate the resulting force via the distance from the anchor */
  float springForce = (mSpringRestLength - springVectorLength) * mSpringConstant ;
  glm::vec3 springVectorForce = glm::normalize(springVector) * springForce;

  if (glm::any(glm::isnan(springVectorForce))) {
    Logger::log(1, "%s error: spring vector contains NaN value(s)\n", __FUNCTION__);
    return;
  }

  body->addForce(springVectorForce * body->getMass());
}
