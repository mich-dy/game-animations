#pragma once

#include "BodyLink.h"

class ContactRod : public BodyLink {
  public:
    ContactRod(const float length);
    virtual unsigned int addContact(std::shared_ptr<BodyContact> contact, const unsigned int contactLimit) override;

  private:
    float mRodLength = 0.0f;
};

