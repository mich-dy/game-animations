#include <cstring>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include "CommandBuffer.h"
#include "Texture.h"
#include "Logger.h"

bool Texture::loadTexture(VkRenderData &renderData, std::string textureFilename, bool generateMipMaps) {
  int texWidth;
  int texHeight;
  int numberOfChannels;

  std::vector<unsigned char*> textureData {};
  textureData.resize(1);

  textureData.at(0) = stbi_load(textureFilename.c_str(), &texWidth, &texHeight, &numberOfChannels, STBI_rgb_alpha);

  /* by default, the value in the vulkan structs needs to be one for the original image */
  uint32_t mipMapLevels = 1;
  if (generateMipMaps) {
    /* add one for the original level, required by Vulkan */
    mipMapLevels += static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight))));
    Logger::log(1, "%s: generating %i extra mipmap levels\n", __FUNCTION__, mipMapLevels - 1);
  }

  if (!textureData.at(0)) {
    Logger::log(1, "%s error: could not load file '%s'\n", __FUNCTION__, textureFilename.c_str());
    stbi_image_free(textureData.at(0));
    return false;
  }

  textureData.resize(mipMapLevels);

  std::vector<VkDeviceSize> imageSizes{};
  imageSizes.resize(mipMapLevels);
  imageSizes.at(0) = texWidth * texHeight * 4;

  /* required for staging copy */
  std::vector<VkExtent3D> textureExtents{};
  textureExtents.resize(mipMapLevels);

  textureExtents.at(0).width = texWidth;
  textureExtents.at(0).height = texHeight;
  textureExtents.at(0).depth = 1;

  if (mipMapLevels > 1) {
    uint32_t mipWidth = texWidth;
    uint32_t mipHeight = texHeight;

    for (unsigned int i = 1; i < mipMapLevels; ++i) {
      textureData.at(i) = stbir_resize_uint8_srgb(textureData.at(i-1), mipWidth, mipHeight, 0,
                                                  nullptr, mipWidth > 1 ? mipWidth / 2 : 1,
                                                  mipHeight > 1 ? mipHeight / 2 : 1, 0, STBIR_RGBA);

      if (mipWidth > 1) {
        mipWidth /= 2;
      }
      if (mipHeight > 1) {
        mipHeight /= 2;
      }

      textureExtents.at(i).width = mipWidth;
      textureExtents.at(i).height = mipHeight;
      textureExtents.at(i).depth = 1;

      imageSizes.at(i) = mipWidth * mipHeight * 4;
      Logger::log(1, "%s: created level %i with width %i and height %i\n", __FUNCTION__, i, mipWidth, mipHeight);
    }
  }


  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = static_cast<uint32_t>(texWidth);
  imageInfo.extent.height = static_cast<uint32_t>(texHeight);
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = mipMapLevels;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

  /* find out if linear blitting is supported */
  VkFormatProperties formatProperties{};
  vkGetPhysicalDeviceFormatProperties(renderData.rdVkbPhysicalDevice.physical_device, imageInfo.format, &formatProperties);

  const bool linearFilterSupported = formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
  if (linearFilterSupported) {
    Logger::log(1, "%s: linear filtering support to use 'vkCmdBlitImage': %s\n", __FUNCTION__, linearFilterSupported ? "yes" : "no");
  }

  VmaAllocationCreateInfo imageAllocInfo{};
  imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  if (vmaCreateImage(renderData.rdAllocator, &imageInfo, &imageAllocInfo, &renderData.rdTextureImage,  &renderData.rdTextureImageAlloc, nullptr) != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate texture image via VMA\n", __FUNCTION__);
    return false;
  }

  /* staging buffers */
  std::vector<VkBufferCreateInfo> stagingBufferInfos{};
  stagingBufferInfos.resize(mipMapLevels);

  std::vector<VkBuffer> stagingBuffers{};
  stagingBuffers.resize(mipMapLevels);

  std::vector<VmaAllocation> stagingBufferAllocs{};
  stagingBufferAllocs.resize(mipMapLevels);

  std::vector<VmaAllocationCreateInfo> stagingAllocInfos{};
  stagingAllocInfos.resize(mipMapLevels);

  /* fill structs */
  for (unsigned int i = 0; i < mipMapLevels; ++i) {
    stagingBufferInfos.at(i).sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfos.at(i).size = imageSizes.at(i);
    stagingBufferInfos.at(i).usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    stagingAllocInfos.at(i).usage = VMA_MEMORY_USAGE_CPU_ONLY;
  }

  for (unsigned int i = 0; i < mipMapLevels; ++i) {
    if (vmaCreateBuffer(renderData.rdAllocator, &stagingBufferInfos.at(i), &stagingAllocInfos.at(i), &stagingBuffers.at(i),
        &stagingBufferAllocs.at(i), nullptr) != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate texture staging buffer %i via VMA\n", __FUNCTION__, i);
      return false;
    }
  }

  /* copy data to staging buffers */
  void* data;
  for (unsigned int i = 0; i < mipMapLevels; ++i) {
    vmaMapMemory(renderData.rdAllocator, stagingBufferAllocs.at(i), &data);
    std::memcpy(data, textureData.at(i), static_cast<uint32_t>(imageSizes.at(i)));
    vmaUnmapMemory(renderData.rdAllocator, stagingBufferAllocs.at(i));
  }

  /* and free images */
  for (unsigned int i = 1; i < mipMapLevels; ++i) {
    stbi_image_free(textureData.at(i));
  }

  VkImageSubresourceRange stagingBufferRange{};
  stagingBufferRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  stagingBufferRange.baseMipLevel = 0;
  stagingBufferRange.levelCount = 1;
  stagingBufferRange.baseArrayLayer = 0;
  stagingBufferRange.layerCount = 1;

  /* 1st barrier, undefined to transfer optimal */
  VkImageMemoryBarrier stagingBufferTransferBarrier{};
  stagingBufferTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  stagingBufferTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  stagingBufferTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  stagingBufferTransferBarrier.image = renderData.rdTextureImage;
  stagingBufferTransferBarrier.subresourceRange = stagingBufferRange;
  stagingBufferTransferBarrier.srcAccessMask = 0;
  stagingBufferTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  /* 2nd barrier, transfer optimal to shader optimal */
  VkImageMemoryBarrier stagingBufferShaderBarrier{};
  stagingBufferShaderBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  stagingBufferShaderBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  stagingBufferShaderBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  stagingBufferShaderBarrier.image = renderData.rdTextureImage;
  stagingBufferShaderBarrier.subresourceRange = stagingBufferRange;
  stagingBufferShaderBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  stagingBufferShaderBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  /* copy command data */
  std::vector<VkBufferImageCopy> stagingBufferCopies{};
  stagingBufferCopies.resize(mipMapLevels);

  for (unsigned int i = 0; i < mipMapLevels; ++i) {
    stagingBufferCopies.at(i).bufferOffset = 0;
    stagingBufferCopies.at(i).bufferRowLength = 0;
    stagingBufferCopies.at(i).bufferImageHeight = 0;
    stagingBufferCopies.at(i).imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    stagingBufferCopies.at(i).imageSubresource.mipLevel = i;
    stagingBufferCopies.at(i).imageSubresource.baseArrayLayer = 0;
    stagingBufferCopies.at(i).imageSubresource.layerCount = 1;
    stagingBufferCopies.at(i).imageExtent = textureExtents.at(i);
  }

  /* create the command buffer for the copy loop */
  VkCommandBuffer uploadCommandBuffer = CommandBuffer::createSingleShotBuffer(renderData);

  for (unsigned int i = 0; i < mipMapLevels; ++i) {
    stagingBufferTransferBarrier.subresourceRange.baseMipLevel = i;
    vkCmdPipelineBarrier(uploadCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &stagingBufferTransferBarrier);

    vkCmdCopyBufferToImage(uploadCommandBuffer, stagingBuffers.at(i), renderData.rdTextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &stagingBufferCopies.at(i));

    stagingBufferShaderBarrier.subresourceRange.baseMipLevel = i;
    vkCmdPipelineBarrier(uploadCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &stagingBufferShaderBarrier);
  }

  bool commandResult = CommandBuffer::submitSingleShotBuffer(renderData, uploadCommandBuffer, renderData.rdGraphicsQueue);

  if (!commandResult) {
    Logger::log(1, "%s error: could not submit texture transfer commands", __FUNCTION__);
    return false;
  }

  /* remove staging buffers */
  for (unsigned int i = 0; i < mipMapLevels; ++i) {
    vmaDestroyBuffer(renderData.rdAllocator, stagingBuffers.at(i), stagingBufferAllocs.at(i));
  }

  /* image view and sampler */
  VkImageViewCreateInfo texViewInfo{};
  texViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  texViewInfo.image = renderData.rdTextureImage;
  texViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  texViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  texViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  texViewInfo.subresourceRange.baseMipLevel = 0;
  texViewInfo.subresourceRange.levelCount = mipMapLevels;
  texViewInfo.subresourceRange.baseArrayLayer = 0;
  texViewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(renderData.rdVkbDevice.device, &texViewInfo, nullptr, &renderData.rdTextureImageView) != VK_SUCCESS) {
    Logger::log(1, "%s error: could not create image view for texture\n", __FUNCTION__);
    return false;
  }

  VkSamplerCreateInfo texSamplerInfo{};
  texSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  texSamplerInfo.magFilter = VK_FILTER_LINEAR;
  texSamplerInfo.minFilter = VK_FILTER_LINEAR;
  texSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  texSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  texSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  texSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  texSamplerInfo.unnormalizedCoordinates = VK_FALSE;
  texSamplerInfo.compareEnable = VK_FALSE;
  texSamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  texSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  texSamplerInfo.mipLodBias = 0.0f;
  texSamplerInfo.minLod = 0.0f;
  texSamplerInfo.maxLod = static_cast<float>(mipMapLevels);
  texSamplerInfo.anisotropyEnable = VK_FALSE;
  texSamplerInfo.maxAnisotropy = 1.0f;

  if (vkCreateSampler(renderData.rdVkbDevice.device, &texSamplerInfo, nullptr, &renderData.rdTextureSampler) != VK_SUCCESS) {
    Logger::log(1, "%s error: could not create sampler for texture\n", __FUNCTION__);
    return false;
  }

  VkDescriptorSetLayoutBinding textureBind{};
  textureBind.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  textureBind.binding = 0;
  textureBind.descriptorCount = 1;
  textureBind.pImmutableSamplers = nullptr;
  textureBind.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo textureCreateInfo{};
  textureCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  textureCreateInfo.bindingCount = 1;
  textureCreateInfo.pBindings = &textureBind;

  if (vkCreateDescriptorSetLayout(renderData.rdVkbDevice.device, &textureCreateInfo, nullptr, &renderData.rdTextureDescriptorLayout) != VK_SUCCESS) {
    Logger::log(1, "%s error: could not create descriptor set layout\n", __FUNCTION__);
    return false;
  }

  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = 1;

  VkDescriptorPoolCreateInfo descriptorPool{};
  descriptorPool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPool.poolSizeCount = 1;
  descriptorPool.pPoolSizes = &poolSize;
  descriptorPool.maxSets = 16;

  if (vkCreateDescriptorPool(renderData.rdVkbDevice.device, &descriptorPool, nullptr, &renderData.rdTextureDescriptorPool) != VK_SUCCESS) {
    Logger::log(1, "%s error: could not create descriptor pool\n", __FUNCTION__);
    return false;
  }

  VkDescriptorSetAllocateInfo descriptorAllocateInfo{};
  descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorAllocateInfo.descriptorPool = renderData.rdTextureDescriptorPool;
  descriptorAllocateInfo.descriptorSetCount = 1;
  descriptorAllocateInfo.pSetLayouts = &renderData.rdTextureDescriptorLayout;

  if (vkAllocateDescriptorSets(renderData.rdVkbDevice.device, &descriptorAllocateInfo, &renderData.rdTextureDescriptorSet) != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate descriptor set\n", __FUNCTION__);
    return false;
  }

  VkDescriptorImageInfo descriptorImageInfo{};
  descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  descriptorImageInfo.imageView = renderData.rdTextureImageView;
  descriptorImageInfo.sampler = renderData.rdTextureSampler;

  VkWriteDescriptorSet writeDescriptorSet{};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeDescriptorSet.dstSet = renderData.rdTextureDescriptorSet;
  writeDescriptorSet.dstBinding = 0;
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.pImageInfo = &descriptorImageInfo;

  vkUpdateDescriptorSets(renderData.rdVkbDevice.device, 1, &writeDescriptorSet, 0, nullptr);

  Logger::log(1, "%s: texture '%s' loaded (%dx%d, %d channels)\n", __FUNCTION__, textureFilename.c_str(), texWidth, texHeight, numberOfChannels);
  return true;
}

void Texture::cleanup(VkRenderData &renderData) {
  vkDestroyDescriptorPool(renderData.rdVkbDevice.device, renderData.rdTextureDescriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(renderData.rdVkbDevice.device, renderData.rdTextureDescriptorLayout, nullptr);
  vkDestroySampler(renderData.rdVkbDevice.device, renderData.rdTextureSampler, nullptr);
  vkDestroyImageView(renderData.rdVkbDevice.device, renderData.rdTextureImageView, nullptr);
  vmaDestroyImage(renderData.rdAllocator, renderData.rdTextureImage, renderData.rdTextureImageAlloc);
}
