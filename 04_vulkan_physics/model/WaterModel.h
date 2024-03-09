#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>

#include "VkRenderData.h"

class WaterModel {
  public:
    VkMesh getVertexData();

    void setPosition(const glm::vec3 pos);
    glm::vec3 getPosition() const;

  private:
    void init();

    VkMesh mVertexData {};
    glm::vec3 mPosition = glm::vec3(0.0f);
};
