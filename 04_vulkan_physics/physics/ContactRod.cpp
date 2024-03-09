#include "ContactRod.h"

ContactRod::ContactRod(const float length) : mRodLength(length) { }

unsigned int ContactRod::addContact(std::shared_ptr<BodyContact> contact, const unsigned int contactLimit) {
  float currentRodLength = getCurrentLength();

  /* desired lendth, do nothing */
  if (currentRodLength == mRodLength) {
    return 0;
  }

  contact->setBody(0, mBodies.at(0));
  contact->setBody(1, mBodies.at(1));

  glm::vec3 contactNormal = glm::normalize(mBodies.at(1)->getPosition() - mBodies.at(0)->getPosition());
  contact->setContactNormal(contactNormal);

  /* extend or compress? */
  if (currentRodLength > mRodLength) {
    contact->setContactNormal(contactNormal);
    contact->setInterPenetration(currentRodLength - mRodLength);
  } else {
    contact->setContactNormal(contactNormal * -1.0f);
    contact->setInterPenetration(mRodLength - currentRodLength);
  }

  /* do not bounce*/
  contact->setRestiutionCoeff(0.0f);

  /* added one contact */
  return 1;
}

