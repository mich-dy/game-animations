#pragma once

#include <unordered_set>
#include <tuple>
#include <memory>

#include "IForceGenerator.h"
#include "RigidBody.h"

struct EntryHash {
  template <typename T, typename U>
  auto operator()(const std::pair<T, U> &p) const -> size_t {
   return std::hash<T>{} (p.first) ^ std::hash<U>{} (p.second);
  }
};

class ForceRegistry {
  public:
    void addEntry(const std::shared_ptr<RigidBody> body, const std::shared_ptr<IForceGenerator> force);
    void deleteEntry(const std::shared_ptr<RigidBody> body, const std::shared_ptr<IForceGenerator> force);
    void clear();

    void updateForces(const float deltaTime);

  private:
    std::unordered_set<std::pair<std::shared_ptr<RigidBody>, std::shared_ptr<IForceGenerator>>, EntryHash> mEntries;
};
