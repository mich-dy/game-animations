#include "Model.h"
#include "Logger.h"

Model::Model() {
  mRigidBody = std::make_shared<RigidBody>();
}

std::shared_ptr<RigidBody> Model::getRigidBody() {
  return mRigidBody;
}

void Model::setPosition(const glm::vec3 pos) {
  mRigidBody->setPosition(pos);
}

glm::vec3 Model::getPosition() const {
  return mRigidBody->getPosition();
}

void Model::setOrientation(const glm::quat orient) {
  mRigidBody->setOrientation(orient);
}

glm::quat Model::getOrientation() const {
  return mRigidBody->getOrientation();
}


void Model::setPhysicsEnabled(const bool value) {
  mPhysicsEnabled = value;
}

void Model::setMass(const float mass) {
  mRigidBody->setMass(mass);
}

void Model::setVelocity(const glm::vec3 velo) {
  mRigidBody->setVelocity(velo);
}

void Model::setAcceleration(const glm::vec3 accel) {
  mRigidBody->setAcceleration(accel);
}

void Model::setLinearDamping(const float damp) {
  mRigidBody->setLinearDaming(damp);
}

void Model::setAngularDaming(const float damp) {
  mRigidBody->setAngularDamping(damp);
}

void Model::addForce(const glm::vec3 force) {
  mRigidBody->addForce(force);
}

void Model::clearAccumulatedForce() {
  mRigidBody->clearAccumulatedForce();
}

void Model::update(float deltaTime) {
  /* TODO: animations etc. */

  /* physics update */
  if (mPhysicsEnabled) {
    mRigidBody->integrate(deltaTime);
  }
}

VkMesh Model::getVertexData() {
  if (mVertexData.vertices.size() == 0) {
    init();
  }
  return mVertexData;
}

void Model::init() {
  // do nothing, overwritten in derived classes
}
