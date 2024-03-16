#include "PipelineLayout.h"
#include "Logger.h"

#include <vector>
#include <VkBootstrap.h>

bool PipelineLayout::init(VkRenderData& renderData, VkPipelineLayout& pipelineLayout) {

  std::vector<VkDescriptorSetLayout> layouts;
  layouts.push_back(renderData.rdTextureDescriptorLayout);
  layouts.push_back(renderData.rdUBODescriptorLayout);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 0;

  if (vkCreatePipelineLayout(renderData.rdVkbDevice.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create pipeline layout\n", __FUNCTION__);
      return false;
  }
  return true;
}

void PipelineLayout::cleanup(VkRenderData &renderData, VkPipelineLayout &pipelineLayout) {
  vkDestroyPipelineLayout(renderData.rdVkbDevice.device, pipelineLayout, nullptr);
}
