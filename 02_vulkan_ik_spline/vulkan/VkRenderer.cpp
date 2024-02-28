#include <imgui_impl_glfw.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/spline.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "VkRenderer.h"
#include "Logger.h"

VkRenderer::VkRenderer(GLFWwindow *window) {
  mRenderData.rdWindow = window;

  mPerspViewMatrices.emplace_back(glm::mat4(1.0f)); // view matrix
  mPerspViewMatrices.emplace_back(glm::mat4(1.0f)); // perspective matrix
}

bool VkRenderer::init(unsigned int width, unsigned int height) {
  mRenderData.rdWidth = width;
  mRenderData.rdHeight = height;

  if (!mRenderData.rdWindow) {
    Logger::log(1, "%s error: invalid GLFWwindow handle\n", __FUNCTION__);
    return false;
  }

  if (!deviceInit()) {
    return false;
  }

  if (!initVma()) {
    return false;
  }

  if (!getQueue()) {
    return false;
  }

  if (!createSwapchain()) {
    return false;
  }

  /* must be done AFTER swapchain as we need data from it */
  if (!createDepthBuffer()) {
    return false;
  }

  if (!createCommandPool()) {
    return false;
  }

  if (!createCommandBuffer()) {
    return false;
  }

  if (!createUBO(mRenderData.rdPerspViewMatrixUBO, mPerspViewMatrices)) {
    return false;
  }
  /* before pipeline layout and pipeline */
  if (!loadGltfModel()) {
      return false;
  }

  if (!createSSBO(mRenderData.rdJointMatrixSSBO, mGltfModel->getJointMatrices())) {
    return false;
  }

  if (!createSSBO(mRenderData.rdJointDualQuatSSBO, mGltfModel->getJointDualQuats())) {
    return false;
  }

  if (!createVBO()) {
    return false;
  }

  if (!createRenderPass()) {
    return false;
  }

  if (!createGltfPipelineLayout()) {
      return false;
  }

  if (!createLinePipeline()) {
      return false;
  }


  if (!createGltfSkeletonPipeline()) {
      return false;
  }

  if (!createGltfGPUPipeline()) {
      return false;
  }

  if (!createGltfGPUDQPipeline()) {
      return false;
  }

  if (!createFramebuffer()) {
    return false;
  }

  if (!createSyncObjects()) {
    return false;
  }

  if (!initUserInterface()) {
    return false;
  }

 /* valid, but emtpy */
  mLineMesh = std::make_shared<VkMesh>();
  Logger::log(1, "%s: line mesh storage initialized\n", __FUNCTION__);

  /* reset skeleton split */
  mRenderData.rdSkelSplitNode = mRenderData.rdModelNodeCount - 1;

  /* set values for inverse kinematics */
  /* hard-code right arm here for startup */
  mRenderData.rdIkEffectorNode = 19;
  mRenderData.rdIkRootNode = 26;
  mGltfModel->setInverseKinematicsNodes(mRenderData.rdIkEffectorNode,
    mRenderData.rdIkRootNode);
  mGltfModel->setNumIKIterations(mRenderData.rdIkIterations);

  mFrameTimer.start();

  Logger::log(1, "%s: Vulkan renderer initialized to %ix%i\n", __FUNCTION__, width, height);
  return true;
}

bool VkRenderer::deviceInit() {
  /* instance and window - we need Vukan 1.1 for the "VK_KHR_maintenance1" extension */
  vkb::InstanceBuilder instBuild;
  auto instRet = instBuild.use_default_debug_messenger().request_validation_layers().require_api_version(1, 1, 0).build();
  if (!instRet) {
    Logger::log(1, "%s error: could not build vkb instance\n", __FUNCTION__);
    return false;
  }
  mRenderData.rdVkbInstance = instRet.value();

  VkResult result = VK_ERROR_UNKNOWN;
  result = glfwCreateWindowSurface(mRenderData.rdVkbInstance, mRenderData.rdWindow, nullptr, &mSurface);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: Could not create Vulkan surface\n", __FUNCTION__);
    return false;
  }

  /* just get the first available device */
  vkb::PhysicalDeviceSelector physicalDevSel{mRenderData.rdVkbInstance};
  auto firstPysicalDevSelRet = physicalDevSel.set_surface(mSurface).select();
  if (!firstPysicalDevSelRet) {
    Logger::log(1, "%s error: could not get physical devices\n", __FUNCTION__);
    return false;
  }

  /* a 2nd call is required to enable all the supported features, like wideLines */
  VkPhysicalDeviceFeatures physFeatures;
  vkGetPhysicalDeviceFeatures(firstPysicalDevSelRet.value(), &physFeatures);

  auto secondPhysicalDevSelRet = physicalDevSel.set_surface(mSurface).set_required_features(physFeatures).select();
  if (!secondPhysicalDevSelRet) {
    Logger::log(1, "%s error: could not get physical devices\n", __FUNCTION__);
    return false;
  }
  mRenderData.rdVkbPhysicalDevice = secondPhysicalDevSelRet.value();

  Logger::log(1, "%s: found physical device '%s'\n", __FUNCTION__, mRenderData.rdVkbPhysicalDevice.name.c_str());

  mMinUniformBufferOffsetAlignment = mRenderData.rdVkbPhysicalDevice.properties.limits.minUniformBufferOffsetAlignment;
  Logger::log(1, "%s: the psyical device as a minimal unifom buffer offset of %i bytes\n", __FUNCTION__, mMinUniformBufferOffsetAlignment);

  vkb::DeviceBuilder devBuilder{mRenderData.rdVkbPhysicalDevice};
  auto devBuilderRet = devBuilder.build();
  if (!devBuilderRet) {
    Logger::log(1, "%s error: could not get devices\n", __FUNCTION__);
    return false;
  }
  mRenderData.rdVkbDevice = devBuilderRet.value();

  return true;
}

bool VkRenderer::getQueue() {
  auto graphQueueRet = mRenderData.rdVkbDevice.get_queue(vkb::QueueType::graphics);
  if (!graphQueueRet.has_value()) {
    Logger::log(1, "%s error: could not get graphics queue\n", __FUNCTION__);
    return false;
  }
  mRenderData.rdGraphicsQueue = graphQueueRet.value();

  auto presentQueueRet = mRenderData.rdVkbDevice.get_queue(vkb::QueueType::present);
  if (!presentQueueRet.has_value()) {
    Logger::log(1, "%s error: could not get present queue\n", __FUNCTION__);
    return false;
  }
  mRenderData.rdPresentQueue = presentQueueRet.value();

  return true;
}

bool VkRenderer::createDepthBuffer() {
  VkExtent3D depthImageExtent = {
        mRenderData.rdVkbSwapchain.extent.width,
        mRenderData.rdVkbSwapchain.extent.height,
        1
  };

  mRenderData.rdDepthFormat = VK_FORMAT_D32_SFLOAT;

  VkImageCreateInfo depthImageInfo{};
  depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
  depthImageInfo.format = mRenderData.rdDepthFormat;
  depthImageInfo.extent = depthImageExtent;
  depthImageInfo.mipLevels = 1;
  depthImageInfo.arrayLayers = 1;
  depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VmaAllocationCreateInfo depthAllocInfo{};
  depthAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  depthAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vmaCreateImage(mRenderData.rdAllocator, &depthImageInfo, &depthAllocInfo, &mRenderData.rdDepthImage, &mRenderData.rdDepthImageAlloc, nullptr) != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate depth buffer memory\n", __FUNCTION__);
    return false;
  }

  VkImageViewCreateInfo depthImageViewinfo{};
  depthImageViewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  depthImageViewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  depthImageViewinfo.image = mRenderData.rdDepthImage;
  depthImageViewinfo.format = mRenderData.rdDepthFormat;
  depthImageViewinfo.subresourceRange.baseMipLevel = 0;
  depthImageViewinfo.subresourceRange.levelCount = 1;
  depthImageViewinfo.subresourceRange.baseArrayLayer = 0;
  depthImageViewinfo.subresourceRange.layerCount = 1;
  depthImageViewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

  if (vkCreateImageView(mRenderData.rdVkbDevice.device, &depthImageViewinfo, nullptr, &mRenderData.rdDepthImageView) != VK_SUCCESS) {
    Logger::log(1, "%s error: could not create depth buffer image view\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createSwapchain() {
  vkb::SwapchainBuilder swapChainBuild{mRenderData.rdVkbDevice};

  /* VK_PRESENT_MODE_FIFO_KHR enables vsync */
  auto  swapChainBuildRet = swapChainBuild.set_old_swapchain(mRenderData.rdVkbSwapchain).set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR).build();
  if (!swapChainBuildRet) {
    Logger::log(1, "%s error: could not init swapchain\n", __FUNCTION__);
    return false;
  }

  vkb::destroy_swapchain(mRenderData.rdVkbSwapchain);
  mRenderData.rdVkbSwapchain = swapChainBuildRet.value();

  return true;
}

bool VkRenderer::recreateSwapchain() {
  /* handle minimize */
  while (mRenderData.rdWidth == 0 || mRenderData.rdHeight == 0) {
    glfwGetFramebufferSize(mRenderData.rdWindow, &mRenderData.rdWidth, &mRenderData.rdHeight);
    glfwWaitEvents();
  }
  vkDeviceWaitIdle(mRenderData.rdVkbDevice.device);

  /* cleanup */
  Framebuffer::cleanup(mRenderData);
  vkDestroyImageView(mRenderData.rdVkbDevice.device, mRenderData.rdDepthImageView, nullptr);
  vmaDestroyImage(mRenderData.rdAllocator, mRenderData.rdDepthImage, mRenderData.rdDepthImageAlloc);

  mRenderData.rdVkbSwapchain.destroy_image_views(mRenderData.rdSwapchainImageViews);

  /* and recreate */
  if (!createSwapchain()) {
    Logger::log(1, "%s error: could not recreate swapchain\n", __FUNCTION__);
    return false;
  }

  if (!createDepthBuffer()) {
    Logger::log(1, "%s error: could not recreate depth buffer\n", __FUNCTION__);
    return false;
  }

  if (!createFramebuffer()) {
    Logger::log(1, "%s error: could not recreate framebuffers\n", __FUNCTION__);
    return false;
  }

  return true;
}

bool VkRenderer::createVBO() {
  /* init with arbitrary size here */
  if (!VertexBuffer::init(mRenderData, mRenderData.rdVertexBufferData, 2000)) {
    Logger::log(1, "%s error: could not create vertex buffer\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createUBO(VkUniformBufferData &UBOData,
  std::vector<glm::mat4> matricesToUpload) {
  if (!UniformBuffer::init(mRenderData, UBOData, matricesToUpload)) {
    Logger::log(1, "%s error: could not create uniform buffers\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createSSBO(VkShaderStorageBufferData &SSBOData,
  std::vector<glm::mat4> matricesToUpload) {
  if (!ShaderStorageBuffer::init(mRenderData, SSBOData, matricesToUpload)) {
    Logger::log(1, "%s error: could not create shader storage buffers\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createSSBO(VkShaderStorageBufferData &SSBOData,
  std::vector<glm::mat2x4> matricesToUpload) {
  if (!ShaderStorageBuffer::init(mRenderData, SSBOData, matricesToUpload)) {
    Logger::log(1, "%s error: could not create shader storage buffers\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createRenderPass() {
  if (!Renderpass::init(mRenderData)) {
    Logger::log(1, "%s error: could not init renderpass\n", __FUNCTION__);
    return false;
  }
  return true;
}
bool VkRenderer::createGltfPipelineLayout() {
    if (!PipelineLayout::init(mRenderData, mGltfRenderData.rdGltfModelTexture,
        mRenderData.rdGltfPipelineLayout)) {
        Logger::log(1, "%s error: could not init pipeline layout\n", __FUNCTION__);
        return false;
    }
    return true;
}

bool VkRenderer::createLinePipeline() {
    std::string vertexShaderFile = "shader/line.vert.spv";
    std::string fragmentShaderFile = "shader/line.frag.spv";
    if (!Pipeline::init(mRenderData, mRenderData.rdGltfPipelineLayout, mRenderData.rdLinePipeline,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST, vertexShaderFile, fragmentShaderFile)) {
        Logger::log(1, "%s error: could not init line shader pipeline\n", __FUNCTION__);
        return false;
    }
    return true;
}

bool VkRenderer::createGltfSkeletonPipeline() {
  std::string vertexShaderFile = "shader/line.vert.spv";
  std::string fragmentShaderFile = "shader/line.frag.spv";
  if (!GltfSkeletonPipeline::init(mRenderData, mRenderData.rdGltfPipelineLayout,
      mRenderData.rdGltfSkeletonPipeline, VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
      vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init gltf skeleton shader pipeline\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createGltfGPUPipeline() {
  std::string vertexShaderFile = "shader/gltf_gpu.vert.spv";
  std::string fragmentShaderFile = "shader/gltf_gpu.frag.spv";
  if (!GltfGPUPipeline::init(mRenderData, mRenderData.rdGltfPipelineLayout,
      mRenderData.rdGltfGPUPipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init gltf GPU shader pipeline\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createGltfGPUDQPipeline() {
  std::string vertexShaderFile = "shader/gltf_gpu_dquat.vert.spv";
  std::string fragmentShaderFile = "shader/gltf_gpu_dquat.frag.spv";
  if (!GltfGPUPipeline::init(mRenderData, mRenderData.rdGltfPipelineLayout,
      mRenderData.rdGltfGPUDQPipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init gltf GPU dual quat shader pipeline\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createFramebuffer() {
  if (!Framebuffer::init(mRenderData)) {
    Logger::log(1, "%s error: could not init framebuffer\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createCommandPool() {
  if (!CommandPool::init(mRenderData)) {
    Logger::log(1, "%s error: could not create command pool\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createCommandBuffer() {
  if (!CommandBuffer::init(mRenderData, mRenderData.rdCommandBuffer)) {
    Logger::log(1, "%s error: could not create command buffers\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createSyncObjects() {
  if (!SyncObjects::init(mRenderData)) {
    Logger::log(1, "%s error: could not create sync objects\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::loadTexture(VkTextureData& textureData) {
  std::string textureFileName = "textures/crate.png";
  if (!Texture::loadTexture(mRenderData, textureData, textureFileName)) {
    Logger::log(1, "%s error: could not load texture\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::initVma() {
  VmaAllocatorCreateInfo allocatorInfo{};
  allocatorInfo.physicalDevice = mRenderData.rdVkbPhysicalDevice.physical_device;
  allocatorInfo.device = mRenderData.rdVkbDevice.device;
  allocatorInfo.instance = mRenderData.rdVkbInstance.instance;
  if (vmaCreateAllocator(&allocatorInfo, &mRenderData.rdAllocator) != VK_SUCCESS) {
    Logger::log(1, "%s error: could not init VMA\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::initUserInterface() {
  if (!mUserInterface.init(mRenderData)) {
    Logger::log(1, "%s error: could not init ImGui\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::loadGltfModel() {
  mGltfModel = std::make_shared<GltfModel>();
  std::string modelFilename = "assets/Woman.gltf";
  std::string modelTexFilename = "textures/Woman.png";
  if (!mGltfModel->loadModel(mRenderData, mGltfRenderData, modelFilename, modelTexFilename)) {
    Logger::log(1, "%s: loading glTF model '%s' failed\n", __FUNCTION__, modelFilename.c_str());
    return false;
  }
  return true;
}

void VkRenderer::cleanup() {
  vkDeviceWaitIdle(mRenderData.rdVkbDevice.device);

  mGltfModel->cleanup(mRenderData, mGltfRenderData);
  mGltfModel.reset();

  mUserInterface.cleanup(mRenderData);

  SyncObjects::cleanup(mRenderData);
  CommandBuffer::cleanup(mRenderData, mRenderData.rdCommandBuffer);
  CommandPool::cleanup(mRenderData);
  Framebuffer::cleanup(mRenderData);
  GltfGPUPipeline::cleanup(mRenderData, mRenderData.rdGltfGPUDQPipeline);
  GltfGPUPipeline::cleanup(mRenderData, mRenderData.rdGltfGPUPipeline);
  GltfSkeletonPipeline::cleanup(mRenderData, mRenderData.rdGltfSkeletonPipeline);
  Pipeline::cleanup(mRenderData, mRenderData.rdLinePipeline);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdGltfPipelineLayout);
  Renderpass::cleanup(mRenderData);
  UniformBuffer::cleanup(mRenderData, mRenderData.rdPerspViewMatrixUBO);
  ShaderStorageBuffer::cleanup(mRenderData, mRenderData.rdJointDualQuatSSBO);
  ShaderStorageBuffer::cleanup(mRenderData, mRenderData.rdJointMatrixSSBO);
  VertexBuffer::cleanup(mRenderData, mRenderData.rdVertexBufferData);

  vkDestroyImageView(mRenderData.rdVkbDevice.device, mRenderData.rdDepthImageView, nullptr);
  vmaDestroyImage(mRenderData.rdAllocator, mRenderData.rdDepthImage, mRenderData.rdDepthImageAlloc);
  vmaDestroyAllocator(mRenderData.rdAllocator);

  mRenderData.rdVkbSwapchain.destroy_image_views(mRenderData.rdSwapchainImageViews);
  vkb::destroy_swapchain(mRenderData.rdVkbSwapchain);

  vkb::destroy_device(mRenderData.rdVkbDevice);
  vkb::destroy_surface(mRenderData.rdVkbInstance.instance, mSurface);
  vkb::destroy_instance(mRenderData.rdVkbInstance);

  Logger::log(1, "%s: Vulkan renderer destroyed\n", __FUNCTION__);
}

void VkRenderer::setSize(unsigned int width, unsigned int height) {
  mRenderData.rdWidth = width;
  mRenderData.rdHeight = height;

  /* Vulkan detects changes and recreates swapchain */
  Logger::log(1, "%s: resized window to %ix%i\n", __FUNCTION__, width, height);
}

void VkRenderer::handleKeyEvents(int key, int scancode, int action, int mods) {
}

void VkRenderer::handleMouseButtonEvents(int button, int action, int mods) {
  /* forward to ImGui */
  ImGuiIO& io = ImGui::GetIO();
  if (button >= 0 && button < ImGuiMouseButton_COUNT) {
    io.AddMouseButtonEvent(button, action == GLFW_PRESS);
  }

  /* hide from application */
  if (io.WantCaptureMouse) {
    return;
  }

  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
    mMouseLock = !mMouseLock;
    if (mMouseLock) {
      glfwSetInputMode(mRenderData.rdWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      /* enable raw mode if possible */
      if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(mRenderData.rdWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
      }
    } else {
      glfwSetInputMode(mRenderData.rdWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
  }
}

void VkRenderer::handleMousePositionEvents(double xPos, double yPos){
  /* forward to ImGui */
  ImGuiIO& io = ImGui::GetIO();
  io.AddMousePosEvent((float)xPos, (float)yPos);

  /* hide from application */
  if (io.WantCaptureMouse) {
    return;
  }

  /* calculate relative movement from last position */
  int mouseMoveRelX = static_cast<int>(xPos) - mMouseXPos;
  int mouseMoveRelY = static_cast<int>(yPos) - mMouseYPos;

  if (mMouseLock) {
    mRenderData.rdViewAzimuth += mouseMoveRelX / 10.0;
    /* keep between 0 and 360 degree */
    if (mRenderData.rdViewAzimuth < 0.0) {
      mRenderData.rdViewAzimuth += 360.0;
    }
    if (mRenderData.rdViewAzimuth >= 360.0) {
      mRenderData.rdViewAzimuth -= 360.0;
    }

    mRenderData.rdViewElevation -= mouseMoveRelY / 10.0;
    /* keep between -89 and +89 degree */
    if (mRenderData.rdViewElevation > 89.0) {
      mRenderData.rdViewElevation = 89.0;
    }
    if (mRenderData.rdViewElevation < -89.0) {
      mRenderData.rdViewElevation = -89.0;
    }
  }

  /* save old values*/
  mMouseXPos = static_cast<int>(xPos);
  mMouseYPos = static_cast<int>(yPos);
}

void VkRenderer::handleMovementKeys() {
  /* hide from application */
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureKeyboard) {
    return;
  }

  mRenderData.rdMoveForward = 0;
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_W) == GLFW_PRESS) {
    mRenderData.rdMoveForward += 1;
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_S) == GLFW_PRESS) {
    mRenderData.rdMoveForward -= 1;
  }

  mRenderData.rdMoveRight = 0;
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_A) == GLFW_PRESS) {
    mRenderData.rdMoveRight -= 1;
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_D) == GLFW_PRESS) {
    mRenderData.rdMoveRight += 1;
  }

  /* viewport Y swap, same as OpenGL */
  mRenderData.rdMoveUp = 0;
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_E) == GLFW_PRESS) {
    mRenderData.rdMoveUp += 1;
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_Q) == GLFW_PRESS) {
    mRenderData.rdMoveUp -= 1;
  }

  /* speed up movement with shift */
  if ((glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
      (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)) {
    mRenderData.rdMoveForward *= 4;
    mRenderData.rdMoveRight *= 4;
    mRenderData.rdMoveUp *= 4;
  }
}

bool VkRenderer::draw() {
  /* get time difference for movement */
  double tickTime = glfwGetTime();
  mRenderData.rdTickDiff = tickTime - mLastTickTime;

  mRenderData.rdFrameTime = mFrameTimer.stop();
  mFrameTimer.start();

  handleMovementKeys();

  if (vkWaitForFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdRenderFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
    Logger::log(1, "%s error: waiting for fence failed\n", __FUNCTION__);
    return false;
  }

  if (vkResetFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdRenderFence) != VK_SUCCESS) {
    Logger::log(1, "%s error:  fence reset failed\n", __FUNCTION__);
    return false;
  }

  uint32_t imageIndex = 0;
  VkResult result = vkAcquireNextImageKHR(mRenderData.rdVkbDevice.device,
      mRenderData.rdVkbSwapchain.swapchain,
      UINT64_MAX,
      mRenderData.rdPresentSemaphore,
      VK_NULL_HANDLE,
      &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return recreateSwapchain();
  } else {
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      Logger::log(1, "%s error: failed to acquire swapchain image. Error is '%i'\n", __FUNCTION__, result);
      return false;
    }
  }

  VkClearValue colorClearValue;
  colorClearValue.color = { { 0.25f, 0.25f, 0.25f, 1.0f } };

  VkClearValue depthValue;
  depthValue.depthStencil.depth = 1.0f;

  VkClearValue clearValues[] = { colorClearValue, depthValue };

  VkRenderPassBeginInfo rpInfo{};
  rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rpInfo.renderPass = mRenderData.rdRenderpass;

  rpInfo.renderArea.offset.x = 0;
  rpInfo.renderArea.offset.y = 0;
  rpInfo.renderArea.extent = mRenderData.rdVkbSwapchain.extent;
  rpInfo.framebuffer = mRenderData.rdFramebuffers[imageIndex];

  rpInfo.clearValueCount = 2;
  rpInfo.pClearValues = clearValues;

  /* use inverted viewport to have same coordinates as OpenGL */
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = static_cast<float>(mRenderData.rdVkbSwapchain.extent.height);
  viewport.width = static_cast<float>(mRenderData.rdVkbSwapchain.extent.width);
  viewport.height = -static_cast<float>(mRenderData.rdVkbSwapchain.extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = { 0, 0 };
  scissor.extent = mRenderData.rdVkbSwapchain.extent;

  mMatrixGenerateTimer.start();
  mPerspViewMatrices.at(0) = mCamera.getViewMatrix(mRenderData);
  mPerspViewMatrices.at(1) = glm::perspective(
    glm::radians(static_cast<float>(mRenderData.rdFieldOfView)),
    static_cast<float>(mRenderData.rdVkbSwapchain.extent.width) /
    static_cast<float>(mRenderData.rdVkbSwapchain.extent.height), 0.01f, 50.0f);

  /* check values and reset model nodes if required */
  static blendMode lastBlendMode = mRenderData.rdBlendingMode;
  if (lastBlendMode != mRenderData.rdBlendingMode) {
    lastBlendMode = mRenderData.rdBlendingMode;
    if (mRenderData.rdBlendingMode != blendMode::additive) {
      mRenderData.rdSkelSplitNode = mRenderData.rdModelNodeCount - 1;
    }
    mGltfModel->resetNodeData();
  }

  static int skelSplitNode = mRenderData.rdSkelSplitNode;
  if (skelSplitNode != mRenderData.rdSkelSplitNode) {
    mGltfModel->setSkeletonSplitNode(mRenderData.rdSkelSplitNode);
    skelSplitNode = mRenderData.rdSkelSplitNode;
    mGltfModel->resetNodeData();
  }

  static ikMode lastIkMode = mRenderData.rdIkMode;
  if (lastIkMode != mRenderData.rdIkMode) {
    mGltfModel->resetNodeData();
    lastIkMode = mRenderData.rdIkMode;
    /* clear timer */
    if (mRenderData.rdIkMode == ikMode::off) {
      mRenderData.rdIKTime = 0.0f;
    }
  }

  static int numIKIterations = mRenderData.rdIkIterations;
  if (numIKIterations != mRenderData.rdIkIterations) {
    mGltfModel->setNumIKIterations(mRenderData.rdIkIterations);
    mGltfModel->resetNodeData();
    numIKIterations = mRenderData.rdIkIterations;
  }

  static int ikEffectorNode = mRenderData.rdIkEffectorNode;
  static int ikRootNode = mRenderData.rdIkRootNode;
  if (ikEffectorNode != mRenderData.rdIkEffectorNode ||
      ikRootNode != mRenderData.rdIkRootNode) {
    mGltfModel->setInverseKinematicsNodes(mRenderData.rdIkEffectorNode,
      mRenderData.rdIkRootNode);
    mGltfModel->resetNodeData();
    ikEffectorNode = mRenderData.rdIkEffectorNode;
    ikRootNode = mRenderData.rdIkRootNode;
  }

  /* animate */
  if (mRenderData.rdPlayAnimation) {
    if (mRenderData.rdBlendingMode == blendMode::crossfade ||
        mRenderData.rdBlendingMode == blendMode::additive) {
      mGltfModel->playAnimation(mRenderData.rdAnimClip,
        mRenderData.rdCrossBlendDestAnimClip, mRenderData.rdAnimSpeed,
        mRenderData.rdAnimCrossBlendFactor, mRenderData.rdAnimationPlayDirection);
    } else {
      mGltfModel->playAnimation(mRenderData.rdAnimClip, mRenderData.rdAnimSpeed,
        mRenderData.rdAnimBlendFactor, mRenderData.rdAnimationPlayDirection);
    }
  } else {
    mRenderData.rdAnimEndTime = mGltfModel->getAnimationEndTime(mRenderData.rdAnimClip);
    if (mRenderData.rdBlendingMode == blendMode::crossfade ||
        mRenderData.rdBlendingMode == blendMode::additive) {
      mGltfModel->crossBlendAnimationFrame(mRenderData.rdAnimClip,
        mRenderData.rdCrossBlendDestAnimClip, mRenderData.rdAnimTimePosition,
        mRenderData.rdAnimCrossBlendFactor);
    } else {
      mGltfModel->blendAnimationFrame(mRenderData.rdAnimClip, mRenderData.rdAnimTimePosition,
        mRenderData.rdAnimBlendFactor);
    }
  }

  /* solve IK */
  if (mRenderData.rdIkMode != ikMode::off) {
    mIKTimer.start();
    switch (mRenderData.rdIkMode) {
      case ikMode::ccd:
        mGltfModel->solveIKByCCD(mRenderData.rdIkTargetPos);
        break;
      case ikMode::fabrik:
        mGltfModel->solveIKByFABRIK(mRenderData.rdIkTargetPos);
        break;
      default:
        /* do nothing */
        break;
    }
    mRenderData.rdIKTime = mIKTimer.stop();
  }

  mLineMesh->vertices.clear();

  /* get gltTF skeleton */
  mSkeletonLineIndexCount = 0;
  if (mRenderData.rdDrawSkeleton) {
    std::shared_ptr<VkMesh> mesh = mGltfModel->getSkeleton();
    mSkeletonLineIndexCount += mesh->vertices.size();
    mLineMesh->vertices.insert(mLineMesh->vertices.begin(),
      mesh->vertices.begin(), mesh->vertices.end());
  }

  /* draw coordiante arrows on target position */
  mCoordArrowsLineIndexCount = 0;
  if ((mRenderData.rdIkMode == ikMode::ccd ||
      mRenderData.rdIkMode == ikMode::fabrik) &&
      mRenderData.rdTargetCoordLines) {
    mCoordArrowsMesh = mCoordArrowsModel.getVertexData();
    mCoordArrowsLineIndexCount = mCoordArrowsMesh.vertices.size();
    std::for_each(mCoordArrowsMesh.vertices.begin(), mCoordArrowsMesh.vertices.end(),
      [=](auto &n){
        n.color /= 2.0f;
        n.position += mRenderData.rdIkTargetPos;
    });

    mLineMesh->vertices.insert(mLineMesh->vertices.end(),
      mCoordArrowsMesh.vertices.begin(), mCoordArrowsMesh.vertices.end());
  }

  /* draw spline */
  mSplineLineIndexCount = 0;
  mSplineMesh.vertices.clear();
  if ((mRenderData.rdIkMode == ikMode::ccd ||
      mRenderData.rdIkMode == ikMode::fabrik) &&
      mRenderData.rdDrawSplineLines) {
    mSplineMesh = mSplineModel.createVertexData(25,
      mRenderData.rdSplineStartVertex, mRenderData.rdSplineStartTangent,
      mRenderData.rdSplineEndVertex, mRenderData.rdSplineEndTangent);
    mSplineLineIndexCount = mSplineMesh.vertices.size();
    mLineMesh->vertices.insert(mLineMesh->vertices.end(),
      mSplineMesh.vertices.begin(), mSplineMesh.vertices.end());
  }

  /* position target on current spline position */
  mRenderData.rdIkTargetPos = glm::hermite(
    mRenderData.rdSplineStartVertex, mRenderData.rdSplineStartTangent,
    mRenderData.rdSplineEndVertex, mRenderData.rdSplineEndTangent,
    mRenderData.rdInterpValue);

  mRenderData.rdMatrixGenerateTime = mMatrixGenerateTimer.stop();

  /* prepare command buffer */
  if (vkResetCommandBuffer(mRenderData.rdCommandBuffer, 0) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to reset command buffer\n", __FUNCTION__);
    return false;
  }

  VkCommandBufferBeginInfo cmdBeginInfo{};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if(vkBeginCommandBuffer(mRenderData.rdCommandBuffer, &cmdBeginInfo) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to begin command buffer\n", __FUNCTION__);
    return false;
  }

  /* upload data to VBO */
  mUploadToVBOTimer.start();

  if (mLineMesh->vertices.size() > 0) {
    VertexBuffer::uploadData(mRenderData, mRenderData.rdVertexBufferData, *mLineMesh);
  }

  if (mModelUploadRequired) {
    /* upload glTF model data */
    mGltfModel->uploadVertexBuffers(mRenderData, mGltfRenderData);
    mGltfModel->uploadIndexBuffer(mRenderData, mGltfRenderData);
    mModelUploadRequired = false;
  }

  mRenderData.rdUploadToVBOTime = mUploadToVBOTimer.stop();

  /* the rendering itself happens here */
  vkCmdBeginRenderPass(mRenderData.rdCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

  /* required for dynamic viewport */
  vkCmdSetViewport(mRenderData.rdCommandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(mRenderData.rdCommandBuffer, 0, 1, &scissor);

  /* UBOs */
  vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    mRenderData.rdGltfPipelineLayout, 1, 1, &mRenderData.rdPerspViewMatrixUBO.rdUBODescriptorSet,
    0, nullptr);
  vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    mRenderData.rdGltfPipelineLayout, 2, 1, &mRenderData.rdJointMatrixSSBO.rdSSBODescriptorSet,
    0, nullptr);
  vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    mRenderData.rdGltfPipelineLayout, 3, 1, &mRenderData.rdJointDualQuatSSBO.rdSSBODescriptorSet,
    0, nullptr);

  /* draw glTF model */
  if (mRenderData.rdDrawGltfModel) {
    mGltfModel->draw(mRenderData, mGltfRenderData);
  }

  if (mCoordArrowsLineIndexCount > 0 || mSkeletonLineIndexCount > 0 || mSplineLineIndexCount > 0) {
    /* line and box vertex buffer */
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(mRenderData.rdCommandBuffer, 0, 1,
      &mRenderData.rdVertexBufferData.rdVertexBuffer, &offset);
    vkCmdSetLineWidth(mRenderData.rdCommandBuffer, 3.0f);
  }

  /* draw the coordinate arrow WITH depth buffer */
  if (mCoordArrowsLineIndexCount > 0) {
    vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      mRenderData.rdLinePipeline);
    vkCmdDraw(mRenderData.rdCommandBuffer, mCoordArrowsLineIndexCount, 1,
      mSkeletonLineIndexCount, 0);
  }

  if (mSplineLineIndexCount > 0) {
    vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      mRenderData.rdLinePipeline);
    vkCmdDraw(mRenderData.rdCommandBuffer, mSplineLineIndexCount, 1,
      mCoordArrowsLineIndexCount + mSkeletonLineIndexCount, 0);
  }

  /* draw the skeleton last, disable depth test to overlay */
  if (mSkeletonLineIndexCount > 0) {
    vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      mRenderData.rdGltfSkeletonPipeline);
    vkCmdDraw(mRenderData.rdCommandBuffer, mSkeletonLineIndexCount, 1, 0, 0);
  }

  /* imgui overlay */
  mUIGenerateTimer.start();
  mUserInterface.createFrame(mRenderData);
  mRenderData.rdUIGenerateTime = mUIGenerateTimer.stop();

  mUIDrawTimer.start();
  mUserInterface.render(mRenderData);
  mRenderData.rdUIDrawTime = mUIDrawTimer.stop();

  vkCmdEndRenderPass(mRenderData.rdCommandBuffer);

  if (vkEndCommandBuffer(mRenderData.rdCommandBuffer) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to end command buffer\n", __FUNCTION__);
    return false;
  }

  /* upload UBO data after commands are created */
  mUploadToUBOTimer.start();
  void* data;
  vmaMapMemory(mRenderData.rdAllocator, mRenderData.rdPerspViewMatrixUBO.rdUboBufferAlloc,
    &data);
  memcpy(data, mPerspViewMatrices.data(),
    static_cast<uint32_t>(mPerspViewMatrices.size() * sizeof(glm::mat4)));
  vmaUnmapMemory(mRenderData.rdAllocator, mRenderData.rdPerspViewMatrixUBO.rdUboBufferAlloc);

  if (mRenderData.rdGPUDualQuatVertexSkinning == skinningMode::dualQuat) {
    std::vector<glm::mat2x4> jointDualQuats = mGltfModel->getJointDualQuats();
    vmaMapMemory(mRenderData.rdAllocator, mRenderData.rdJointDualQuatSSBO.rdSsboBufferAlloc,
      &data);
    memcpy(data, jointDualQuats.data(),
      static_cast<uint32_t>(jointDualQuats.size() * sizeof(glm::mat2x4)));
    vmaUnmapMemory(mRenderData.rdAllocator, mRenderData.rdJointDualQuatSSBO.rdSsboBufferAlloc);
  } else {
    std::vector<glm::mat4> jointMatrices = mGltfModel->getJointMatrices();
    vmaMapMemory(mRenderData.rdAllocator, mRenderData.rdJointMatrixSSBO.rdSsboBufferAlloc,
      &data);
    memcpy(data, jointMatrices.data(),
      static_cast<uint32_t>(jointMatrices.size() * sizeof(glm::mat4)));
    vmaUnmapMemory(mRenderData.rdAllocator, mRenderData.rdJointMatrixSSBO.rdSsboBufferAlloc);
  }
  mRenderData.rdUploadToUBOTime = mUploadToUBOTimer.stop();

  /* submit command buffer */
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  submitInfo.pWaitDstStageMask = &waitStage;

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &mRenderData.rdPresentSemaphore;

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &mRenderData.rdRenderSemaphore;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &mRenderData.rdCommandBuffer;

  if (vkQueueSubmit(mRenderData.rdGraphicsQueue, 1, &submitInfo, mRenderData.rdRenderFence) != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to submit draw command buffer\n", __FUNCTION__);
    return false;
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &mRenderData.rdRenderSemaphore;

  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &mRenderData.rdVkbSwapchain.swapchain;

  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(mRenderData.rdPresentQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    return recreateSwapchain();
  } else {
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: failed to present swapchain image\n", __FUNCTION__);
      return false;
    }
  }
  mLastTickTime = tickTime;

  return true;
}
