#include "BodyLink.h"

#include <glm/glm.hpp>

#include "Logger.h"

float BodyLink::getCurrentLength() const {
  if (!mBodies.at(0) || !mBodies.at(1)) {
    Logger::log(1, "%s error: no body/bodies found\n", __FUNCTION__);
    return 0.0f;
  }

  return glm::length(mBodies.at(0)->getPosition() - mBodies.at(1)->getPosition());
}

void BodyLink::setBody(const unsigned int index, const std::shared_ptr<RigidBody> body) {
  if (index > mBodies.size()) {
    Logger::log(1, "%s error: invalid body index %i\n", __FUNCTION__, index);
    return;
  }

  mBodies.at(index) = body;
}

void BodyLink::addBodies(const std::shared_ptr<RigidBody> firstBody, const std::shared_ptr<RigidBody> secondBody) {
  mBodies.at(0) = firstBody;
  mBodies.at(1) = secondBody;

  Logger::log(1, "%s: added bodies with mass %f and %f to body link\n", __FUNCTION__, mBodies.at(0)->getMass(), mBodies.at(1)->getMass());
}
