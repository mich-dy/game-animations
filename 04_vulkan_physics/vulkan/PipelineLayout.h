/* Vulkan Pipeline Layout */
#pragma once

#include <vector>
#include "VkRenderData.h"

class PipelineLayout {
  public:
    static bool init(VkRenderData& renderData, VkPipelineLayout& pipelineLayout, std::vector<VkDescriptorSetLayout>& layouts);
    static void cleanup(VkRenderData &renderData, VkPipelineLayout &pipelineLayout);
};
