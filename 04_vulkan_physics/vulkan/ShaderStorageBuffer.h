#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VkRenderData.h"

class ShaderStorageBuffer {
public:
  static bool createDescriptorPool(VkRenderData &renderData);
  static bool init(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, size_t bufferSize);
  static void uploadData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, std::vector<glm::mat4> matricesToUpload);
  static void cleanup(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData);
};
