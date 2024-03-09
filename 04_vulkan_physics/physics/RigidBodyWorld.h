#pragma once

#include <vector>
#include <memory>

#include "RigidBody.h"
#include "BodyContact.h"
#include "ContactResolver.h"
#include "IContactGenerator.h"

#include "VkRenderData.h"

class RigidBodyWorld {
  public:
    RigidBodyWorld(const unsigned int maxContacts, const unsigned int numIterations = 0);
    void startFrame();

    void integrate(const float deltaTime);
    unsigned int generateContacts();
    void runPhysics(VkRenderData& renderData, const float deltaTime);

    void addRigidBody(const std::shared_ptr<RigidBody> newBody);

    bool addCableContact(const std::shared_ptr< RigidBody > firstBody, const std::shared_ptr< RigidBody > secondBody, const float length, const float restitutionFactor);

    bool addRodContact(const std::shared_ptr< RigidBody > firstBody, const std::shared_ptr< RigidBody > secondBody, const float length);

    std::shared_ptr<RigidBody> getRigidBody(const unsigned int index);

  private:
    std::shared_ptr<ContactResolver> mResolver = nullptr;

    std::vector<std::shared_ptr<RigidBody>> mBodies{};
    /* stores the contact generators, (like a cable, containing the two connected bodies) */
    std::vector<std::shared_ptr<IContactGenerator>> mContactGenerators{};

    /* stores generated contacts */
    std::vector<std::shared_ptr<BodyContact>> mBodyContacts{};

    unsigned int mMaxContacts = 0;
    unsigned int mNumIterations = 0;
};
