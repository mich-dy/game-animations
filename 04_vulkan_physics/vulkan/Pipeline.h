/* Vulkan graphics pipeline with shaders */
#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class Pipeline {
  public:
    static bool init(VkRenderData &renderData, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline, VkPrimitiveTopology topology,
                    VkVertexInputBindingDescription mainBinding, std::vector<VkVertexInputAttributeDescription> attributes,
                    std::string vertexShaderFilename, std::string fragmentShaderFilename);
    static void cleanup(VkRenderData &renderData, VkPipeline &pipeline);
};
