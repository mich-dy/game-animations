#pragma once

#include <array>
#include <memory>

#include "RigidBody.h"
#include "IContactGenerator.h"

class BodyLink : public IContactGenerator {
  public:
    float getCurrentLength() const;
    // virtual unsigned int addContact(std::shared_ptr<BodyContact>, const unsigned int contactLimit) = 0; //coumentation only

    void setBody(const unsigned int index, const std::shared_ptr<RigidBody> body);
    void addBodies(const std::shared_ptr<RigidBody> firstBody, const std::shared_ptr<RigidBody> secondBody);

  protected:
    std::array<std::shared_ptr<RigidBody>, 2> mBodies{};
};
