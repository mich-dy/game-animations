/* Vulkan uniform buffer object */
#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class UniformBuffer {
  public:
    static bool createDescriptorPool(VkRenderData &renderData);
    static bool init(VkRenderData &renderData, const VkDeviceSize bufferSize);
    static void uploadData(VkRenderData &renderData, std::vector <glm::mat4> &matrices);
    static void cleanup(VkRenderData &renderData);
};
