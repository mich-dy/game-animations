#include "ContactResolver.h"

#include <array>

ContactResolver::ContactResolver(const unsigned int numIterations) : mNumInterations(numIterations) { }

void ContactResolver::setIterations(const unsigned int numIterations) {
  mNumInterations = numIterations;
}

unsigned int ContactResolver::getNumUsedIterations() const {
  return mUsedIterations;
}

unsigned int ContactResolver::resolveContacts(std::vector<std::shared_ptr<BodyContact>>& contacts, const unsigned int numContacts, const float deltaTime) {
  mUsedIterations = 0;

  while (mUsedIterations < mNumInterations) {
    /* TODO: heap/prio queue or similar to have the entries already sorted? */
    /* or other ways to get the highest separating velocity? */

    /* find contact with max closing velocity */
    float maxValue = FLT_MAX;
    unsigned int maxIndex = numContacts;

    for (unsigned int i = 0; i < numContacts; ++i) {
      float separationVelocity = contacts.at(i)->calculateSeparatingVelocity();
      if (separationVelocity < maxValue && (separationVelocity < 0 || contacts.at(i)->getInterPenetration() > 0)) {
        maxValue = separationVelocity;
        maxIndex = i;
      }
    }

    /* nothing found to resolve, return */
    if (maxIndex == numContacts) {
      break;
    }

    /* resolve the contact between the bodies */
    contacts.at(maxIndex)->resolveContact(deltaTime);

    /* and move the bodies according to the values from contact resolving  */
    /* TODO: this is a quite expensive search, just to find the two rigid bodies.
     * Maybe some sort of hash maps would help? */
    std::array<glm::vec3, 2> bodyMovements = contacts.at(maxIndex)->getBodyMovements();

    for (unsigned int i = 0; i < numContacts; ++i) {
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
  return mUsedIterations;
}

