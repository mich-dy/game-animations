#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class RigidBody {
  public:
    void integrate(const float deltaTime);

    void setPosition(const glm::vec3 pos);
    glm::vec3 getPosition() const;

    void setOrientation(const glm::quat orient);
    glm::quat getOrientation() const;

    void setMass(const float mass);
    float getMass() const;
    bool hasInfiniteMass();
    float getInverseMass() const;

    void setVelocity(const glm::vec3 velo);
    glm::vec3 getVelocity() const;

    void setAcceleration(const glm::vec3 accel);
    glm::vec3 getAcceleration() const;

    void setLinearDaming(const float damp);
    void setAngularDamping(const float damp);

    void addForce(const glm::vec3 force);
    void addTorque(const glm::vec3 torque);

    /* add force in world space to point in world space (we need force and torque) */
    void addForceToWorldPoint(const glm::vec3 force, const glm::vec3 point);
    /* add force in world space to point in body space */
    void addForceToBodyPoint(const glm::vec3 force, const glm::vec3 point);

    void clearAccumulatedForce();

    /* update transform matrix  */
    void calculateDerivedData();

  private:
    void calculateTransformMatrix(const glm::vec3& position, const glm::quat& orientation);
    void setInertiaTensor(const glm::mat3& tensorMatrix);
    void transformInertiaTensorToWorldSpace();

    glm::vec3 convertBodyToWorldSpace(const glm::vec3 bodyPoint);

    /* using the inverse of the mass is easier (i.e., inverse zero -> infinit mass) */
    float mInverseMass = 0.0f;

    /* linear position */
    glm::vec3 mPosition = glm::vec3(0.0f);
    /* linear velocity */
    glm::vec3 mVelocity = glm::vec3(0.0f);

    glm::vec3 mAcceleration = glm::vec3(0.0f);

    float mLinearDamping = 1.0;

    glm::vec3 mAccumulatedForce = glm::vec3(0.0f);

    /* angular orientation */
    glm::quat mOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    /* angular velocity */
    glm::vec3 mRotation = glm::vec3(0.0f);

    float mAngularDamping = 1.0;

    glm::vec3 mAccumulatedTorque = glm::vec3(0.0f);

    /* convert body to world/world to body space */
    glm::mat4 mTransformMatrix = glm::mat4(1.0f);

    /* store inverse tensor for easier usage, in body space coords */
    //glm::mat3 mInverseInertiaTensor = glm::mat3(1.0f);
    glm::mat3 mInverseInertiaTensor = glm::inverse(glm::mat3(1.0f));
    /* the same tensor in world space coordinates */
    glm::mat3 mInverseInertiaTensorWorldSpace = glm::inverse(glm::mat3(1.0f));

    bool mIsAwake = true;
};
