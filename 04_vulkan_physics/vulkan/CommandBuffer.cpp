#include "CommandBuffer.h"
#include "Logger.h"

#include <VkBootstrap.h>

bool CommandBuffer::init(VkRenderData &renderData, VkCommandBuffer &commandBuffer) {
  VkCommandBufferAllocateInfo bufferAllocInfo{};
  bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  bufferAllocInfo.commandPool = renderData.rdCommandPool;
  bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  bufferAllocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(renderData.rdVkbDevice.device, &bufferAllocInfo, &commandBuffer) != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate command buffers\n", __FUNCTION__);
    return false;
  }

  return true;
}

void CommandBuffer::cleanup(VkRenderData &renderData, VkCommandBuffer commandBuffer) {
  vkFreeCommandBuffers(renderData.rdVkbDevice.device, renderData.rdCommandPool, 1, &commandBuffer);
}

VkCommandBuffer CommandBuffer::createSingleShotBuffer(VkRenderData& renderData) {
  Logger::log(2, "%s: creating a single shot command buffer\n", __FUNCTION__);VkCommandBuffer commandBuffer;

  if (!init(renderData, commandBuffer)) {
    Logger::log(1, "%s error: could not create command buffer\n", __FUNCTION__);
    return VK_NULL_HANDLE;
  }

  if (vkResetCommandBuffer(commandBuffer, 0) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to reset command buffer\n", __FUNCTION__);
    return VK_NULL_HANDLE;
  }

  VkCommandBufferBeginInfo cmdBeginInfo{};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if(vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to begin command buffer\n", __FUNCTION__);
    return VK_NULL_HANDLE;
  }

  Logger::log(2, "%s: single shot command buffer successfully created\n", __FUNCTION__);
  return commandBuffer;
}

bool CommandBuffer::submitSingleShotBuffer(VkRenderData& renderData, const VkCommandBuffer commandBuffer, const VkQueue queue) {
  Logger::log(2, "%s: submitting single shot command buffer\n", __FUNCTION__);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to end command buffer\n", __FUNCTION__);
    return false;
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  VkFence bufferFence;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (vkCreateFence(renderData.rdVkbDevice.device, &fenceInfo, nullptr, &bufferFence) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to create buffer fence\n", __FUNCTION__);
    return false;
  }

  if (vkResetFences(renderData.rdVkbDevice.device, 1, &bufferFence) != VK_SUCCESS) {
    Logger::log(1, "%s error: buffer fence reset failed\n", __FUNCTION__);
    return false;
  }

  if (vkQueueSubmit(queue, 1, &submitInfo, bufferFence) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to submit buffer copy command buffer\n", __FUNCTION__);
    return false;
  }

  if (vkWaitForFences(renderData.rdVkbDevice.device, 1, &bufferFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
    Logger::log(1, "%s error: waiting for buffer fence failed\n", __FUNCTION__);
    return false;
  }

  vkDestroyFence(renderData.rdVkbDevice.device, bufferFence, nullptr);
  cleanup(renderData, commandBuffer);

  Logger::log(2, "%s: single shot command buffer successfully submitted\n", __FUNCTION__);
  return true;
}
