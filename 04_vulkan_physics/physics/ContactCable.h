#pragma once

#include "BodyLink.h"

class ContactCable : public BodyLink {
  public:
    ContactCable(const float maxLength, const float restitution);
    virtual unsigned int addContact(std::shared_ptr<BodyContact> contact, const unsigned int contactLimit) override;

  private:
    float mMaxCableLength = 0.0f;
    /* "bouncy-ness" */
    float mCableRestitution = 0.0f;
};
