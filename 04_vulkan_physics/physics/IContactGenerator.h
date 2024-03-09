#pragma once

#include <memory>

#include "BodyContact.h"

class IContactGenerator {
  public:
    /* returns max number of contacts written */
    virtual unsigned int addContact(std::shared_ptr<BodyContact> contact, const unsigned int contactLimit) = 0;
    virtual ~IContactGenerator() = default;
};
