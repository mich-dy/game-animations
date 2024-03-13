#include "RigidBody.h"
#include "Logger.h"


void RigidBody::setMass(const float mass) {
  /* infinite mass */
  if (mass <= 0.0f) {
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

float RigidBody::getInverseMass() const {
  return mInverseMass;
}

void RigidBody::setPosition(const glm::vec3 pos) {
  mPosition = pos;
}

glm::vec3 RigidBody::getPosition() const {
  return mPosition;
}

void RigidBody::setOrientation(const glm::quat orient) {
  mOrientation = orient;
}

glm::quat RigidBody::getOrientation() const {
  return mOrientation;
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

glm::vec3 RigidBody::getAcceleration() const {
  return mAcceleration;
}

void RigidBody::setLinearDaming(const float damp) {
  mLinearDamping = damp;
}

void RigidBody::setAngularDamping(const float damp) {
  mAngularDamping = damp;
}

void RigidBody::addForce(const glm::vec3 force) {
  mAccumulatedForce += force;
  mIsAwake = true;
}

void RigidBody::addTorque(const glm::vec3 torque) {
  mAccumulatedTorque += torque;
  mIsAwake = true;
}

void RigidBody::addForceToBodyPoint(const glm::vec3 force, const glm::vec3 point) {
  addForceToWorldPoint(force, convertBodyToWorldSpace(point));
}

void RigidBody::addForceToWorldPoint(const glm::vec3 force, const glm::vec3 point) {
  mAccumulatedForce += force;
  mAccumulatedTorque += glm::cross(point - mPosition, force);

  mIsAwake = true;
}

glm::vec3 RigidBody::convertBodyToWorldSpace(const glm::vec3 bodyPoint) {
  return mTransformMatrix * glm::vec4(bodyPoint, 1.0f);
}

void RigidBody::clearAccumulatedForce() {
  mAccumulatedForce = glm::vec3(0.0f);
  mAccumulatedTorque = glm::vec3(0.0f);
}

void RigidBody::calculateDerivedData() {
  mOrientation = glm::normalize(mOrientation);

  calculateTransformMatrix(mPosition, mOrientation);

  transformInertiaTensorToWorldSpace();
}

void RigidBody::calculateTransformMatrix(const glm::vec3& position, const glm::quat& orientation) {
  /* quaternion delivers rotation part only */
  mTransformMatrix = glm::mat4_cast(orientation);

  /* add translation to the matrix */
  mTransformMatrix = glm::translate(mTransformMatrix, position);
}

void RigidBody::setInertiaTensor(const glm::mat3& tensorMatrix) {
  mInverseInertiaTensor = glm::inverse(tensorMatrix);
}

void RigidBody::transformInertiaTensorToWorldSpace() {
  mInverseInertiaTensorWorldSpace = glm::mat3(mTransformMatrix) * glm::inverse(mInverseInertiaTensor);
}

void RigidBody::integrate(const float deltaTime) {
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

  /* agular counterpart */
  glm::vec3 angularAcceleration = glm::vec4(mAccumulatedTorque, 1.0f) * mInverseInertiaTensorWorldSpace;
  // glm::vec3 angularAcceleration = mInverseInertiaTensorWorldSpace * glm::vec4(mAccumulatedTorque, 1.0f);

  glm::vec3 scaledAngularAccelerateion = angularAcceleration * deltaTime;
  mRotation += scaledAngularAccelerateion;

  /* apply damping (i.e. drag by air) */
  mVelocity *= std::pow(mLinearDamping, deltaTime);
  mRotation *= std::pow(mAngularDamping, deltaTime);

  /* scale the velocity according to the delta time and update position */
  glm::vec3 scaledVelocity = mVelocity * deltaTime;
  mPosition += scaledVelocity;

  glm::vec3 scaledRotation = mRotation * deltaTime;
  glm::quat rotationQuat = glm::normalize(glm::quat(1.0f, scaledRotation));
  mOrientation *= rotationQuat;

  /* clear summed up force */
  // now done at the start of a frame
  // clearAccumulatedForce();
  // calculateDerivedData();
}
