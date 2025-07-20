/* rotation arrows */
#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "OGLRenderData.h"

class RotationArrowsModel {
  public:
    OGLMesh getVertexData();

  private:
    void init();
    OGLMesh mVertexData{};
};
