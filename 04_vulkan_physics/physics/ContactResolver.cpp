#include "ContactResolver.h"

#include <array>

ContactResolver::ContactResolver(const unsigned int numIterations) : mNumInterations(numIterations) { }

void ContactResolver::setIterations(const unsigned int numIterations) {
  mNumInterations = numIterations;
}

unsigned int ContactResolver::getNumUsedIterations() const {
  return mUsedIterations;
}

void ContactResolver::resolveContacts(std::vector<std::shared_ptr<BodyContact>>& contacts, const unsigned int maxContacts, const float deltaTime) {
  mUsedIterations = 0;

  while (mUsedIterations < mNumInterations) {
    /* find contact with max closing velocity */
    float maxValue = FLT_MAX;
    unsigned int maxIndex = maxContacts;

    for (unsigned int i = 0; i < maxIndex; ++i) {
      float separationVelocity = contacts.at(i)->calculateSeparatingVelocity();
      if (separationVelocity < maxValue && (separationVelocity < 0 || contacts.at(i)->getInterPenetration() > 0)) {
        maxValue = separationVelocity;
        maxIndex = i;
      }
    }

    /* nothing found to resolve, return */
    if (maxIndex == maxContacts) {
      break;
    }

    /* resolve the contact between the bodies */
    contacts.at(maxIndex)->resolveContact(deltaTime);

    /* and move the two bodies according to the values from contact resolving  */
    std::array<glm::vec3, 2> bodyMovements = contacts.at(maxIndex)->getBodyMovements();
    for (unsigned int i = 0; i < maxContacts; ++i) {
      if (contacts.at(i)->getBody(0) == contacts.at(maxIndex)->getBody(0)) {
        contacts.at(i)->setInterPenetration(
          contacts.at(i)->getInterPenetration() - glm::dot(bodyMovements.at(0), contacts.at(i)->getContactNormal()));
      } else if (contacts.at(i)->getBody(0) == contacts.at(maxIndex)->getBody(1)) {
        contacts.at(i)->setInterPenetration(
          contacts.at(i)->getInterPenetration() - glm::dot(bodyMovements.at(1), contacts.at(i)->getContactNormal()));
      }

      if (contacts.at(i)->getBody(1)) {
        if (contacts.at(i)->getBody(1) == contacts.at(maxIndex)->getBody(0)) {
          contacts.at(i)->setInterPenetration(
            contacts.at(i)->getInterPenetration() + glm::dot(bodyMovements.at(0), contacts.at(i)->getContactNormal()));
        } else if (contacts.at(i)->getBody(1) == contacts.at(maxIndex)->getBody(1)) {
          contacts.at(i)->setInterPenetration(
            contacts.at(i)->getInterPenetration() + glm::dot(bodyMovements.at(1), contacts.at(i)->getContactNormal()));
        }
      }
    }

    ++mUsedIterations;
  }
}

