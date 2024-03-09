#pragma once

#include <memory>
#include <vector>

#include "BodyContact.h"

class ContactResolver {
  public:
    ContactResolver(const unsigned int numIterations);
    void setIterations(const unsigned int numIterations);
    unsigned int getNumUsedIterations() const;

    /* only work on up to numContacts contacts, the remaining entries may be invalid */
    void resolveContacts(std::vector<std::shared_ptr<BodyContact>>& contacts, const unsigned int numContacts, const float deltaTime);

  private:
    unsigned int mNumInterations = 0;
    unsigned int mUsedIterations = 0;
};
