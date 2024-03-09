#include "BuoyancyForce.h"

#include <glm/glm.hpp>
#include "Logger.h"

BuoyancyForce::BuoyancyForce(const float maxDepth, const float bodyVolume, const float waterHeight, const float liquidDensity) :
  mMaxSubmersionDepth(maxDepth), mBodyVolume(bodyVolume), mWaterYHeight(waterHeight), mLiquidDensity(liquidDensity) {}

void BuoyancyForce::updateForce(std::shared_ptr<RigidBody> body, float deltaTime) {
  if (!body) {
    Logger::log(1, "%s error: no body given\n", __FUNCTION__);
    return;
  }

  if (body->hasInfiniteMass()) {
    return;
  }

  float objectDepth = body->getPosition().y;

  /* object is not in water */
  if (objectDepth >= mWaterYHeight + mMaxSubmersionDepth) {
    return;
  }

  glm::vec3 buoyancyForce = glm::vec3(0.0f);

  /* completely under water, max force */
  if (objectDepth <= mWaterYHeight - mMaxSubmersionDepth) {
    buoyancyForce.y = mLiquidDensity * mBodyVolume;
  } else {
    buoyancyForce.y = mLiquidDensity * mBodyVolume * (objectDepth - mMaxSubmersionDepth - mWaterYHeight) / (2 * mMaxSubmersionDepth);
  }

  body->addForce(buoyancyForce * body->getMass());
}
