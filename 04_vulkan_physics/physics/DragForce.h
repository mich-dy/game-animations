#pragma once

#include "IForceGenerator.h"

class DragForce : public IForceGenerator {
  public:
    DragForce(const float linearCoeff, const float squareCoeff);
    virtual ~DragForce() = default;

    virtual void updateForce(std::shared_ptr<RigidBody> body, float deltaTime) override;

  private:
    float mLinearDragCoefficient = 0.0f;
    float mSquareDragCoefficient = 0.0f;
};
