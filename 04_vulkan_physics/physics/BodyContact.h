#pragma once

#include <memory>
#include <array>
#include <glm/glm.hpp>

#include "RigidBody.h"

class BodyContact {
  public:
    void resolveContact(const float deltaTime);
    float calculateSeparatingVelocity() const;

    float getInterPenetration() const;
    void setInterPenetration(const float value);

    void resolveInterPenetration(float deltaTime);

    float getRestitutionCoeff() const;
    void setRestiutionCoeff(const float value);

    std::shared_ptr<RigidBody> getBody(const unsigned int index) const;
    void setBody(const unsigned int index, const std::shared_ptr<RigidBody> body);

    std::array<glm::vec3, 2> getBodyMovements() const;

    void setContactNormal(const glm::vec3 normal);
    glm::vec3 getContactNormal() const;

    void setContactPoint(const glm::vec3 point);
    glm::vec3 getContactPoint() const;

  private:
    void resolveVelocity(const float deltaTime);

    std::array<std::shared_ptr<RigidBody>, 2> mBodies {};
    std::array<glm::vec3, 2> mBodyMovement{};

    /* bouncy-ness */
    float mRestitutionCoefficient = 0.0f;

    /* point of contact in world coords */
    glm::vec3 mContactPoint = glm::vec3(0.0f);
    /* contact direction, also in world coords */
    glm::vec3 mContactNormal = glm::vec3(0.0f);
    /* amount of penetration between the two bodies */
    float mInterPenetration = 0;
};
