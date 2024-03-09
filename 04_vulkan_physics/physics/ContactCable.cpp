#include "ContactCable.h"

ContactCable::ContactCable(const float maxLength, const float restitution) : mMaxCableLength(maxLength), mCableRestitution(restitution) { }

unsigned int ContactCable::addContact(std::shared_ptr<BodyContact> contact, const unsigned int contactLimit) {
  float cableLength = getCurrentLength();

  /* below max length, do nothing, return 0 contacts added */
  if (cableLength < mMaxCableLength) {
    return 0;
  }

  contact->setBody(0, mBodies.at(0));
  contact->setBody(1, mBodies.at(1));

  glm::vec3 contactNormal = glm::normalize(mBodies.at(1)->getPosition() - mBodies.at(0)->getPosition());
  contact->setContactNormal(contactNormal);

  /* amount to bounce back*/
  contact->setInterPenetration(cableLength - mMaxCableLength);
  contact->setRestiutionCoeff(mCableRestitution);

  /* added one contact */
  return 1;
}
