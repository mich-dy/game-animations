/* Vulkan uniform buffer object */
#pragma once

#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class VertexBuffer {
  public:
    static bool init(VkRenderData &renderData, VkVertexBufferData &bufferData);
    static bool uploadData(VkRenderData &renderData, VkVertexBufferData &bufferData, VkMesh vertexData);
    static bool uploadData(VkRenderData &renderData, VkVertexBufferData &bufferData, VkLineMesh vertexData);
    static void cleanup(VkRenderData &renderData, VkVertexBufferData &bufferData);

  private:
    static bool doStagingUpload(VkRenderData &renderData, VkVertexBufferData &bufferData, const unsigned int vertexDataSize);
    static bool resizeBuffer(VkRenderData &renderData, VkVertexBufferData &bufferData, const unsigned int vertexDataSize);
};
