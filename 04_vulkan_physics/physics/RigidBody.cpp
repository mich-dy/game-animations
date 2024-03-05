#include "RigidBody.h"
#include "Logger.h"


void RigidBody::setMass(const float mass) {
  if (mass == 0.0f) {
    Logger::log(1, "%s error: mass cannot be zero\n", __FUNCTION__);
    return;
  }

  /* infinite mass */
  if (mass < 0.0f) {
    mInverseMass = 0.0f;
    return;
  }

  mInverseMass = 1.0f / mass;
}

float RigidBody::getMass() const {
  return 1.0f / mInverseMass;
}

bool RigidBody::hasInfiniteMass() {
  if (mInverseMass <= 0.0f ) {
    return true;
  }
  return false;
}

void RigidBody::setPosition(const glm::vec3 pos) {
  mPosition = pos;
}

glm::vec3 RigidBody::getPosition() const {
  return mPosition;
}

void RigidBody::setVelocity(const glm::vec3 velo) {
  mVelocity = velo;
}

glm::vec3 RigidBody::getVelocity() const {
  return mVelocity;
}

void RigidBody::setAcceleration(const glm::vec3 accel) {
  mAcceleration = accel;
}

void RigidBody::setDaming(const float damp) {
  mDamping = damp;
}

void RigidBody::updatePhysics(const float deltaTime) {
  /* infinite mass, don't do anything */
  if (mInverseMass <= 0.0f) {
    return;
  }

  if (deltaTime <= 0.0f) {
    Logger::log(1, "%s error: deltaTime must be greater than zero\n", __FUNCTION__);
    return;
  }

  glm::vec3 mLastFrameAcceleration = mAcceleration;

  glm::vec3 scaledAccumForce = mAccumulatedForce * mInverseMass;
  mLastFrameAcceleration += scaledAccumForce;

  /* scale acceleration  and add to velocity */
  glm::vec3 scaledAcceleration = mLastFrameAcceleration * deltaTime;
  mVelocity += scaledAcceleration;

  /* apply damping (i.e. drag by air) */
  mVelocity *= std::pow(mDamping, deltaTime);

  /* scale the velocity according to the delta time and update position */
  glm::vec3 scaledVelocity = mVelocity * deltaTime;
  mPosition += scaledVelocity;

  /* clear summed up force */
  clearAccumulatedForce();
}

void RigidBody::addForce(const glm::vec3 force) {
  mAccumulatedForce += force;
}

void RigidBody::clearAccumulatedForce() {
  mAccumulatedForce = glm::vec3(0.0f);
}
