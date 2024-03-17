/* a simple, line based arrow */
#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "VkRenderData.h"

class ArrowModel {
  public:
    VkLineMesh getVertexData();

  private:
    void init();
    VkLineMesh mVertexData;
};
