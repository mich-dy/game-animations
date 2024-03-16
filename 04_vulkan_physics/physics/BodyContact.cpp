#include "BodyContact.h"
#include "Logger.h"

void BodyContact::resolveContact(const float deltaTime) {
  resolveVelocity(deltaTime);
  resolveInterPenetration(deltaTime);
}

float BodyContact::getInterPenetration() const {
  return mInterPenetration;
}

void BodyContact::setInterPenetration(const float value) {
  mInterPenetration = value;
}

void BodyContact::setRestiutionCoeff(const float value) {
  mRestitutionCoefficient = value;
}

float BodyContact::getRestitutionCoeff() const {
  return mRestitutionCoefficient;
}

std::array<glm::vec3, 2> BodyContact::getBodyMovements() const {
  return mBodyMovement;
}

void BodyContact::setContactNormal(const glm::vec3 normal) {
  mContactNormal = glm::normalize(normal);
}

glm::vec3 BodyContact::getContactNormal() const{
  return mContactNormal;
}

void BodyContact::setContactPoint(const glm::vec3 point) {
  mContactPoint = point;
}

glm::vec3 BodyContact::getContactPoint() const {
  return mContactPoint;
}

std::shared_ptr<RigidBody> BodyContact::getBody(const unsigned int index) const {
  if (index > mBodies.size()) {
    Logger::log(1, "%s error: tried to access beyound the body array size\n", __FUNCTION__);
    return nullptr;
  }

  if (!mBodies.at(index)) {
    Logger::log(1, "%s error: no body at index %i\n", __FUNCTION__, index);
    return nullptr;
  }

  return mBodies.at(index);
}

void BodyContact::setBody(const unsigned int index, const std::shared_ptr<RigidBody> body) {
  if (index > mBodies.size()) {
    Logger::log(1, "%s error: tried to access beyound the body array size\n", __FUNCTION__);
    return;
  }

  if (!body) {
    Logger::log(1, "%s error: no body given\n", __FUNCTION__);
    return;
  }

  mBodies.at(index) = body;
}

float BodyContact::calculateSeparatingVelocity() const {
  if (!mBodies.at(0)) {
    Logger::log(1, "%s error: no first rigid body\n", __FUNCTION__);
    return 0.0f;
  }

  glm::vec3 relativeVelocity = mBodies.at(0)->getVelocity();
  if (mBodies.at(1)) {
    relativeVelocity -= mBodies.at(1)->getVelocity();
  }
  return glm::dot(relativeVelocity, mContactNormal);
}

void BodyContact::resolveVelocity(const float deltaTime) {
  if (!mBodies.at(0)) {
    Logger::log(1, "%s error: no first rigid body\n", __FUNCTION__);
    return;
  }

  float separatingVelocity = calculateSeparatingVelocity();

  /* stationary or separating */
  if (separatingVelocity > 0) {
    return;
  }

  float newSeparationVelocity = -separatingVelocity * mRestitutionCoefficient;

  /* check accumulated velocity buildup due to accelration in direction of the contact normal (i.e., sliding on ground) */
  glm::vec3 accumulatedVelocity = mBodies.at(0)->getAcceleration();
  if (mBodies.at(1)) {
    accumulatedVelocity -= mBodies.at(1)->getAcceleration();
  }

  float accumulatedSeparationVelocity = glm::dot(accumulatedVelocity, mContactNormal) * deltaTime;

  if (accumulatedSeparationVelocity < 0) {
    newSeparationVelocity += mRestitutionCoefficient * accumulatedSeparationVelocity;

    /* can't go negative */
    if (newSeparationVelocity < 0) {
      newSeparationVelocity = 0.0f;
    }
  }

  float deltaVelocity = newSeparationVelocity - separatingVelocity;

  /* get total inverse mass to distribute the velocity update proportional */
  float totalInverseMass = mBodies.at(0)->getInverseMass();
  if (mBodies.at(1)) {
    totalInverseMass += mBodies.at(1)->getInverseMass();
  }

  /* infinite masses, do nothing */
  if (totalInverseMass <= 0) {
    return;
  }

  /* impulse to apply */
  float impulse = deltaVelocity / totalInverseMass;
  glm::vec3 impulsePerInverseMass = mContactNormal * impulse;

  /* set velocity in direction of contact, proportional to the masses */
  mBodies.at(0)->setVelocity(mBodies.at(0)->getVelocity() + impulsePerInverseMass * mBodies.at(0)->getInverseMass());

  /* the opposite body gets a negative velocity */
  if (mBodies.at(1)) {
    mBodies.at(1)->setVelocity(mBodies.at(1)->getVelocity() + impulsePerInverseMass * -mBodies.at(1)->getInverseMass());
  }
}

void BodyContact::resolveInterPenetration(float deltaTime) {
  /* nothing to do */
  if (mInterPenetration <= 0.0f) {
    Logger::log(2, "%s: no interpenetration, do nothing\n", __FUNCTION__);
    return;
  }

  if (!mBodies.at(0)) {
    Logger::log(1, "%s error: no first rigid body\n", __FUNCTION__);
    return;
  }

  /* get total inverse mass to distribute the velocity update proportional */
  float totalInverseMass = mBodies.at(0)->getInverseMass();
  if (mBodies.at(1)) {
    totalInverseMass += mBodies.at(1)->getInverseMass();
  }

  /* infinite masses, do nothing */
  if (totalInverseMass <= 0) {
    return;
  }

  glm::vec3 movePerInverseMass = mContactNormal * (mInterPenetration / totalInverseMass);
  /* calculate the movement needed to separate the bodies */
  mBodyMovement.at(0) = movePerInverseMass * mBodies.at(0)->getInverseMass();
  if (mBodies.at(1)) {
    mBodyMovement.at(1) = movePerInverseMass * -mBodies.at(1)->getInverseMass();
  } else {
    mBodyMovement.at(1) = glm::vec3(0.0f);
  }

  /* apply inter-penetration resolution */
  mBodies.at(0)->setPosition(mBodies.at(0)->getPosition() + mBodyMovement.at(0));

  if (mBodies.at(1)) {
    mBodies.at(1)->setPosition(mBodies.at(1)->getPosition() + mBodyMovement.at(1));
  }
}

