#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "VkRenderData.h"
#include "RigidBody.h"

class Model {
  public:
    Model();
    virtual ~Model() = default;

    VkMesh getVertexData();
    void setPhysicsEnabled(const bool value);

    void update(float deltaTime);

    glm::vec3 getPosition() const ;
    void setPosition(const glm::vec3 pos);

    glm::quat getOrientation() const;
    void setOrientation(const glm::quat orient);

    void setMass(const float mass);
    void setVelocity(const glm::vec3 velo);
    void setAcceleration(const glm::vec3 accel);

    void setLinearDamping(const float damp);
    void setAngularDaming(const float damp);

    void addForce(const glm::vec3 force);
    /* TODO: is this needed? */
    void clearAccumulatedForce();

    std::shared_ptr<RigidBody> getRigidBody();

  protected:
    VkMesh mVertexData {};

  private:
    virtual void init();

    bool mPhysicsEnabled = false;

    std::shared_ptr<RigidBody> mRigidBody = nullptr;
};
