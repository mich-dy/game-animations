#pragma once

#include "IForceGenerator.h"

class BuoyancyForce : public IForceGenerator {
  public:
    BuoyancyForce(const float maxDepth, const float bodyVolume, const float waterHeight, const float liquidDensity = 1000.0f);

    virtual void updateForce(std::shared_ptr<RigidBody> body, float deltaTime) override;

  private:
    /* TODO: make these two variables independent of the body */
    float mMaxSubmersionDepth = 0;
    float mBodyVolume = 0;

    float mWaterYHeight = 0;
    float mLiquidDensity = 0;

    /* damping */
    //float mLiquidity = 0.95f;
};
