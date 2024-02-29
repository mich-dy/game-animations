#include <GravityForce.h>

void GravityForce::updateForce(RigidBody& body, float deltaTime) {
  if (body.hasInfiniteMass()) {
    return;
  }

  body.addForce(mGravityConstant * body.getMass());
}
