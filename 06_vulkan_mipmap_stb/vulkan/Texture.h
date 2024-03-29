/* Vulkan texture */
#pragma once

#include <string>
#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class Texture {
  public:
    static bool loadTexture(VkRenderData &renderData, std::string textureFilename, bool generateMipMaps = true);
    static void cleanup(VkRenderData &renderData);
};
