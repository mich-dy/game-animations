#include "ForceRegistry.h"

#include "Logger.h"

void ForceRegistry::addEntry(const std::shared_ptr<RigidBody> body, const std::shared_ptr<IForceGenerator> force) {
  mEntries.emplace(std::make_pair(body, force));
}

void ForceRegistry::deleteEntry(const std::shared_ptr<RigidBody> body, const std::shared_ptr<IForceGenerator> force) {
  mEntries.erase(std::make_pair(body, force));
}

void ForceRegistry::clear() {
  mEntries.clear();
}

void ForceRegistry::updateForces(const float deltaTime) {
  for (const auto& [body, force] : mEntries) {
    force->updateForce(body, deltaTime);
  }
}
