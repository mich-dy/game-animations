#include "RigidBodyWorld.h"
#include <algorithm>

#include "Logger.h"
#include "ContactCable.h"
#include "ContactRod.h"

RigidBodyWorld::RigidBodyWorld(const unsigned int maxContacts, const unsigned int numIterations) : mMaxContacts(maxContacts), mNumIterations(numIterations) {
  mResolver = std::make_shared<ContactResolver>(numIterations);

  /* resize and init array */
  mBodyContacts.resize(maxContacts);
  for (auto& contact: mBodyContacts){
    contact = std::make_shared<BodyContact>();
  }
}

/* clear accumulated forces of all bodies */
void RigidBodyWorld::startFrame() {
  for (auto& body : mBodies) {
    body->clearAccumulatedForce();
    body->calculateDerivedData();
  }
}

void RigidBodyWorld::integrate(const float deltaTime) {
  for (auto& body : mBodies) {
    body->integrate(deltaTime);
  }
}

void RigidBodyWorld::runPhysics(VkRenderData& renderData, const float deltaTime) {
  integrate(deltaTime);

  unsigned int usedContacts = generateContacts();
  renderData.rdContactsIssued = usedContacts;

  if (usedContacts) {
    if (!mNumIterations) {
      mResolver->setIterations(usedContacts * 2);
    }
    renderData.rdContactResolverIterations = mResolver->resolveContacts(mBodyContacts, usedContacts, deltaTime);
  }
}

void RigidBodyWorld::addRigidBody(const std::shared_ptr<RigidBody> newBody) {
  mBodies.emplace_back(newBody);
}

std::shared_ptr<RigidBody> RigidBodyWorld::getRigidBody(const unsigned int index) {
  if (index > mBodies.size()) {
    Logger::log(1, "%s error: tried to access non-existing rigid body\n", __FUNCTION__);
    return nullptr;
  }

  return mBodies.at(index);
}

bool RigidBodyWorld::addCableContact(const std::shared_ptr<RigidBody> firstBody, const std::shared_ptr<RigidBody> secondBody, const float length, const float restitutionFactor) {
  if (std::find(mBodies.begin(), mBodies.end(), firstBody) == mBodies.end() || std::find(mBodies.begin(), mBodies.end(), secondBody) == mBodies.end()) {
    Logger::log(1, "%s error: could not find the rigid bodies in mBodies vector\n", __FUNCTION__);
    return false;
  }

  std::shared_ptr<ContactCable> cable = std::make_shared<ContactCable>(length, restitutionFactor);
  cable->addBodies(firstBody, secondBody);

  mContactGenerators.emplace_back(cable);

  return true;
}

bool RigidBodyWorld::addRodContact(const std::shared_ptr<RigidBody> firstBody, const std::shared_ptr<RigidBody> secondBody, const float length) {
  if (std::find(mBodies.begin(), mBodies.end(), firstBody) == mBodies.end() || std::find(mBodies.begin(), mBodies.end(), secondBody) == mBodies.end()) {
    Logger::log(1, "%s error: could not find the rigid bodies in mBodies vector\n", __FUNCTION__);
    return false;
  }

  std::shared_ptr<ContactRod> rod = std::make_shared<ContactRod>(length);
  rod->addBodies(firstBody, secondBody);

  mContactGenerators.emplace_back(rod);

  return true;
}

unsigned int RigidBodyWorld::generateContacts() {
  unsigned int limit = mMaxContacts;
  unsigned int contactIndex = 0;

  for (auto& generator : mContactGenerators) {
    unsigned int newContacts = generator->addContact(mBodyContacts.at(contactIndex), limit);
    limit -= newContacts;
    contactIndex += newContacts;

    /* over limit, stop iterating  */
    if (limit <= 0 ) {
      break;
    }
  }

  /* return number of contacts generated*/
  return mMaxContacts - limit;
}
