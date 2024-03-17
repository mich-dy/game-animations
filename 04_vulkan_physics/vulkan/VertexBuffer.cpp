#include "VertexBuffer.h"
#include "CommandBuffer.h"
#include "Logger.h"

bool VertexBuffer::init(VkRenderData &renderData, VkVertexBufferData &bufferData) {
  /* vertex buffer */
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferData.rdVertexBufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo bufferAllocInfo{};
  bufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  if (vmaCreateBuffer(renderData.rdAllocator, &bufferInfo, &bufferAllocInfo,
      &bufferData.rdVertexBuffer, &bufferData.rdVertexBufferAlloc, nullptr) != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate vertex buffer via VMA\n", __FUNCTION__);
    return false;
  }

  /* staging buffer for copy */
  VkBufferCreateInfo stagingBufferInfo{};
  stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferInfo.size = bufferData.rdVertexBufferSize;;
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VmaAllocationCreateInfo stagingAllocInfo{};
  stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

  if (vmaCreateBuffer(renderData.rdAllocator, &stagingBufferInfo, &stagingAllocInfo,
      &bufferData.rdVertexStagingBuffer,
      &bufferData.rdVertexStagingBufferAlloc, nullptr) != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate vertex staging buffer via VMA\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VertexBuffer::uploadData(VkRenderData &renderData, VkVertexBufferData &bufferData, VkMesh vertexData) {
  unsigned int vertexDataSize = vertexData.vertices.size() * sizeof(VkVertex);

  /* buffer too small? try to resize */
  if (!resizeBuffer(renderData, bufferData, vertexDataSize)) {
    return false;
  }

  /* copy data to staging buffer*/
  void* data;
  vmaMapMemory(renderData.rdAllocator, bufferData.rdVertexStagingBufferAlloc, &data);
  memcpy(data, vertexData.vertices.data(), vertexDataSize);
  vmaUnmapMemory(renderData.rdAllocator, bufferData.rdVertexStagingBufferAlloc);

  return doStagingUpload(renderData, bufferData, vertexDataSize);
}

bool VertexBuffer::uploadData(VkRenderData &renderData, VkVertexBufferData &bufferData, VkLineMesh vertexData) {
  unsigned int vertexDataSize = vertexData.vertices.size() * sizeof(VkLineVertex);

  /* buffer too small? try to resize */
  if (!resizeBuffer(renderData, bufferData, vertexDataSize)) {
    return false;
  }

  /* copy data to staging buffer*/
  void* data;
  vmaMapMemory(renderData.rdAllocator, bufferData.rdVertexStagingBufferAlloc, &data);
  memcpy(data, vertexData.vertices.data(), vertexDataSize);
  vmaUnmapMemory(renderData.rdAllocator, bufferData.rdVertexStagingBufferAlloc);

  return doStagingUpload(renderData, bufferData, vertexDataSize);
}

bool VertexBuffer::resizeBuffer(VkRenderData& renderData, VkVertexBufferData& bufferData, const unsigned int vertexDataSize) {
  if (bufferData.rdVertexBufferSize < vertexDataSize) {
    bufferData.rdVertexBufferSize = vertexDataSize;
    cleanup(renderData, bufferData);

    if (!init(renderData, bufferData)) {
      Logger::log(1, "%s error: could not create vertex buffer of size %i bytes\n", __FUNCTION__, vertexDataSize);
      return false;
    }
  }
  return true;
}

bool VertexBuffer::doStagingUpload(VkRenderData& renderData, VkVertexBufferData& bufferData, const unsigned int vertexDataSize) {
  VkBufferMemoryBarrier vertexBufferBarrier{};
  vertexBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  vertexBufferBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  vertexBufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
  vertexBufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  vertexBufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  vertexBufferBarrier.buffer = bufferData.rdVertexStagingBuffer;
  vertexBufferBarrier.offset = 0;
  vertexBufferBarrier.size = vertexDataSize;

  VkBufferCopy stagingBufferCopy{};
  stagingBufferCopy.srcOffset = 0;
  stagingBufferCopy.dstOffset = 0;
  stagingBufferCopy.size = vertexDataSize;

  vkCmdCopyBuffer(renderData.rdCommandBuffer, bufferData.rdVertexStagingBuffer,
    bufferData.rdVertexBuffer, 1, &stagingBufferCopy);
  vkCmdPipelineBarrier(renderData.rdCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &vertexBufferBarrier, 0, nullptr);

  return true;
}

void VertexBuffer::cleanup(VkRenderData &renderData, VkVertexBufferData &bufferData) {
  vmaDestroyBuffer(renderData.rdAllocator, bufferData.rdVertexStagingBuffer,
    bufferData.rdVertexStagingBufferAlloc);
  vmaDestroyBuffer(renderData.rdAllocator, bufferData.rdVertexBuffer,
    bufferData.rdVertexBufferAlloc);
}
