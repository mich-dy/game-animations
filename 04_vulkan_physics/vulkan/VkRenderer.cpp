#include <imgui_impl_glfw.h>

#include <glm/gtc/matrix_transform.hpp>
#include <tuple>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "GravityForce.h"
#include "DragForce.h"
#include "AnchoredSpringForce.h"
#include "AnchoredBungeeForce.h"
#include "BuoyancyForce.h"

#include "VkRenderer.h"
#include "Logger.h"

VkRenderer::VkRenderer(GLFWwindow *window) {
  mRenderData.rdWindow = window;

  /* identity matrices */
  mMatrices.viewMatrix = glm::mat4(1.0f);
  mMatrices.projectionMatrix = glm::mat4(1.0f);
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

  /* we need the command pool */
  if (!loadTexture()) {
    return false;
  }

  if (!createUBO()) {
      return false;
  }

  if (!createVBO()) {
      return false;
  }

  if (!createRenderPass()) {
    return false;
  }

  if (!createPipelineLayout()) {
    return false;
  }

  if (!createBasicPipeline()) {
    return false;
  }

  if (!createLinePipeline()) {
    return false;
  }

  if (!createFlatPipeline()) {
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

  mBoxModel = std::make_shared<BoxModel>();
  Logger::log(1, "%s: box model initialized\n", __FUNCTION__);

  mSphereModel = std::make_shared<SphereModel>(0.75f, 12, 24, glm::vec3(0.1f, 0.6f, 0.3f));
  Logger::log(1, "%s: sphere model initialized\n", __FUNCTION__);

  if (!initModel()) {
    Logger::log(1, "%s error: model init failed\n", __FUNCTION__);
    return false;
  }

  mQuatModelMesh = std::make_unique<VkMesh>();
  Logger::log(1, "%s: model mesh storage initialized\n", __FUNCTION__);

  mSphereModelMesh = std::make_unique<VkMesh>();
  Logger::log(1, "%s: sphere model mesh storage initialized\n", __FUNCTION__);


  /* bridge anchors */
  float xPos = 2.0f;
  for (unsigned int i = 0; i < NUMBER_OF_BRIDGE_POINTS * 2; ++i) {
    std::shared_ptr<RigidBody> anchor = std::make_shared<RigidBody>();
    float zPos = 0.0f;

    if (i % 2 == 0) {
      xPos += 1.0f;
      zPos = 1.0f;
    }
    anchor->setPosition(glm::vec3(xPos, 2.0f, zPos));
    anchor->setMass(-1.0f); // infinte mass, do not move
    anchor->setVelocity(glm::vec3(0.0f));
    anchor->setAcceleration(glm::vec3(0.0f));
    anchor->setLinearDaming(0.95f);

    mRigidBodyWorld.addRigidBody(anchor);
  }

  /* plank holders */
  xPos = 2.0f;
  for (unsigned int i = 0; i < NUMBER_OF_BRIDGE_POINTS * 2; ++i) {
    std::shared_ptr<RigidBody> body = std::make_shared<RigidBody>();
    float zPos = 0.0f;

    if (i % 2 == 0) {
      xPos += 1.0f;
      zPos = 1.0f;
    }

    body->setPosition(glm::vec3(xPos, 0.0f, zPos));
    body->setMass(1.0f);
    body->setVelocity(glm::vec3(0.0f));
    body->setAcceleration(glm::vec3(0.0f));
    body->setLinearDaming(0.95f);

    mRigidBodyWorld.addRigidBody(body);
  }

  /* connecten cables from anchors to planks */
  for (unsigned int i = 0; i < NUMBER_OF_BRIDGE_POINTS * 2; ++i) {
    mRigidBodyWorld.addCableContact(mRigidBodyWorld.getRigidBody(i), mRigidBodyWorld.getRigidBody(i + NUMBER_OF_BRIDGE_POINTS * 2), 2.1f, 0.25f);
  }

  /* connection cables between plank holders */
  for (unsigned int i = NUMBER_OF_BRIDGE_POINTS * 2; i < NUMBER_OF_BRIDGE_POINTS * 4 - 2; ++i) {
    mRigidBodyWorld.addCableContact(mRigidBodyWorld.getRigidBody(i), mRigidBodyWorld.getRigidBody(i + 2), 0.75f, 0.1f);
    //mRigidBodyWorld.addRodContact(mRigidBodyWorld.getRigidBody(i), mRigidBodyWorld.getRigidBody(i + 2), 0.75f);
  }

  /* connection rods btween planks  */
  for (unsigned int i = NUMBER_OF_BRIDGE_POINTS * 2; i < NUMBER_OF_BRIDGE_POINTS * 4; i += 2) {
    mRigidBodyWorld.addRodContact(mRigidBodyWorld.getRigidBody(i), mRigidBodyWorld.getRigidBody(i + 1), 0.75f);
  }

  std::shared_ptr<GravityForce> gravity = std::make_shared<GravityForce>(glm::vec3(0.0f, -10.0f, 0.0f));
  mForceRegistry.addEntry(mBoxModel->getRigidBody(), gravity);
  mForceRegistry.addEntry(mSphereModel->getRigidBody(), gravity);

  mWindForce = std::make_shared<WindForce>(glm::vec3(4.0f, 0.0f, 4.0f));
  mForceRegistry.addEntry(mBoxModel->getRigidBody(), mWindForce);
  mForceRegistry.addEntry(mSphereModel->getRigidBody(), mWindForce);

  for (unsigned int i = 0; i < NUMBER_OF_BRIDGE_POINTS * 4; ++i) {
   mForceRegistry.addEntry(mRigidBodyWorld.getRigidBody(i), gravity);
   mForceRegistry.addEntry(mRigidBodyWorld.getRigidBody(i), mWindForce);
  }

  mRigidBodyWorld.addRigidBody(mBoxModel->getRigidBody());
  mRigidBodyWorld.addCableContact(mRigidBodyWorld.getRigidBody(NUMBER_OF_BRIDGE_POINTS * 3 - 1), mBoxModel->getRigidBody(), 2.0f, 0.4f);

  mRigidBodyWorld.addRigidBody(mSphereModel->getRigidBody());
  mRigidBodyWorld.addCableContact(mRigidBodyWorld.getRigidBody(NUMBER_OF_BRIDGE_POINTS * 3), mSphereModel->getRigidBody(), 2.5f, 0.8f);

  /*
  std::shared_ptr<DragForce> drag = std::make_shared<DragForce>(0.25f, 0.01f);
  mForceRegistry.addEntry(mBoxModel->getRigidBody(), drag);
  */
  /* buoyancy does not work correctly yet */
  //std::shared_ptr<BuoyancyForce> buoyancy = std::make_shared<BuoyancyForce>(1.0f, 1.0f, 0.0f);
  //mForceRegistry.addEntry(mBoxModel->getRigidBody(), buoyancy);

  /*
  std::shared_ptr<AnchoredBungeeForce> spring1 = std::make_shared<AnchoredBungeeForce>(mSpring1AnchorPos, 15.0f, 3.0f);
  mForceRegistry.addEntry(mBoxModel->getRigidBody(), spring1);

  std::shared_ptr<AnchoredBungeeForce> spring2 = std::make_shared<AnchoredBungeeForce>(mSpring2AnchorPos, 15.0f, 3.0f);
  mForceRegistry.addEntry(mBoxModel->getRigidBody(), spring2);

  std::shared_ptr<AnchoredBungeeForce> spring3 = std::make_shared<AnchoredBungeeForce>(mSpring3AnchorPos, 15.0f, 3.0f);
  mForceRegistry.addEntry(mBoxModel->getRigidBody(), spring3);
  */

  mAllMeshes = std::make_unique<VkMesh>();
  Logger::log(1, "%s: global mesh storage initialized\n", __FUNCTION__);

  mFrameTimer.start();

  Logger::log(1, "%s: Vulkan renderer initialized to %ix%i\n", __FUNCTION__, width, height);
  return true;
}

bool VkRenderer::initModel() {
  mBoxModel->setPosition(mQuatModelInitialPos);
  mBoxModel->setMass(5.0f);
  mBoxModel->setVelocity(glm::vec3(0.0f));
  mBoxModel->setAcceleration(glm::vec3(0.0f));
  mBoxModel->setLinearDamping(0.75f);
  mBoxModel->setAngularDaming(0.5f);

  mBoxModel->setPhysicsEnabled(true);

  mSphereModel->setPosition(mSphereModelInitialPos);
  mSphereModel->setMass(4.0f);
  mSphereModel->setVelocity(glm::vec3(0.0f));
  mSphereModel->setAcceleration(glm::vec3(0.0f));
  mSphereModel->setLinearDamping(0.75f);
  mSphereModel->setAngularDaming(0.4f);

  mSphereModel->setPhysicsEnabled(true);

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
  if (!VertexBuffer::init(mRenderData)) {
    Logger::log(1, "%s error: could not create vertex buffer\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createUBO() {
  if (!UniformBuffer::init(mRenderData)) {
    Logger::log(1, "%s error: could not create uniform buffers\n", __FUNCTION__);
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

bool VkRenderer::createPipelineLayout() {
  if (!PipelineLayout::init(mRenderData, mRenderData.rdPipelineLayout)) {
    Logger::log(1, "%s error: could not init pipeline layout\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createBasicPipeline() {
  std::string vertexShaderFile = "shader/basic.vert.spv";
  std::string fragmentShaderFile = "shader/basic.frag.spv";
  if (!Pipeline::init(mRenderData, mRenderData.rdPipelineLayout, mRenderData.rdBasicPipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init basic shader pipeline\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createLinePipeline() {
  std::string vertexShaderFile = "shader/line.vert.spv";
  std::string fragmentShaderFile = "shader/line.frag.spv";
  if (!Pipeline::init(mRenderData, mRenderData.rdPipelineLayout, mRenderData.rdLinePipeline, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init line shader pipeline\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createFlatPipeline() {
  std::string vertexShaderFile = "shader/flat.vert.spv";
  std::string fragmentShaderFile = "shader/flat.frag.spv";
  if (!Pipeline::init(mRenderData, mRenderData.rdPipelineLayout, mRenderData.rdFlatPipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init flat shader pipeline\n", __FUNCTION__);
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

bool VkRenderer::loadTexture() {
  std::string textureFileName = "textures/crate.png";
  if (!Texture::loadTexture(mRenderData, textureFileName)) {
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

void VkRenderer::cleanup() {
  vkDeviceWaitIdle(mRenderData.rdVkbDevice.device);

  mUserInterface.cleanup(mRenderData);

  Texture::cleanup(mRenderData);
  SyncObjects::cleanup(mRenderData);
  CommandBuffer::cleanup(mRenderData, mRenderData.rdCommandBuffer);
  CommandPool::cleanup(mRenderData);
  Framebuffer::cleanup(mRenderData);
  Pipeline::cleanup(mRenderData, mRenderData.rdFlatPipeline);
  Pipeline::cleanup(mRenderData, mRenderData.rdLinePipeline);
  Pipeline::cleanup(mRenderData, mRenderData.rdBasicPipeline);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdPipelineLayout);
  Renderpass::cleanup(mRenderData);
  UniformBuffer::cleanup(mRenderData);
  VertexBuffer::cleanup(mRenderData);

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

bool VkRenderer::draw(const float deltaTime) {
  /* no update on zero diff */
  if (deltaTime == 0.0f) {
    return true;
  }

  mRenderData.rdFrameTime = mFrameTimer.stop();
  mFrameTimer.start();

  handleMovementKeys();

  mAllMeshes->vertices.clear();

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
  mMatrices.projectionMatrix = glm::perspective(
    glm::radians(static_cast<float>(mRenderData.rdFieldOfView)),
    static_cast<float>(mRenderData.rdVkbSwapchain.extent.width) /
    static_cast<float>(mRenderData.rdVkbSwapchain.extent.height), 0.01f, 50.0f);

  mCamera.updateCamera(mRenderData, deltaTime);
  mMatrices.viewMatrix = mCamera.getViewMatrix(mRenderData);

  mRenderData.rdMatrixGenerateTime = mMatrixGenerateTimer.stop();

  /* reset all values to zero when UI button is pressed */
  if (mRenderData.rdResetAnglesAndPosition) {
    mRenderData.rdResetAnglesAndPosition = false;

    mQuatModelOrientation = glm::quat();

    initModel();
  }

  /* update physics */
  mPhysicsTimer.start();
  if (mRenderData.rdPhysicsEnabled) {
    mRigidBodyWorld.startFrame();
    mWindForce->enable(mRenderData.rdPhysicsWindEnabled);

    mForceRegistry.updateForces(deltaTime);

    mRigidBodyWorld.runPhysics(mRenderData, deltaTime);
  }

  mRenderData.rdPhysicsTime = mPhysicsTimer.stop();


  mQuatModelPos = mBoxModel->getPosition();
  mRenderData.rdBoxModelPosition = mQuatModelPos;

  mQuatModelOrientation = mBoxModel->getOrientation();
  /* conjugate = same length, but opposite direction*/
  mQuatModelOrientConjugate = glm::conjugate(mQuatModelOrientation);

  mSphereModelPos = mSphereModel->getPosition();
  mRenderData.rdSphereModelPosition = mSphereModelPos;

  mSphereModelOrientation = mSphereModel->getOrientation();
  mSphereModelOrientConjugate = glm::conjugate(mSphereModelOrientation);

  /* draw a static coordinate system */
  mCoordArrowsMesh.vertices.clear();
  if (mRenderData.rdDrawWorldCoordArrows) {
    mCoordArrowsMesh = mCoordArrowsModel.getVertexData();
    std::for_each(mCoordArrowsMesh.vertices.begin(), mCoordArrowsMesh.vertices.end(),
      [=](auto &n){
        n.color /= 2.0f;
    });
    mAllMeshes->vertices.insert(mAllMeshes->vertices.end(),
      mCoordArrowsMesh.vertices.begin(), mCoordArrowsMesh.vertices.end());
  }

  mQuatArrowMesh.vertices.clear();
  mSphereArrowMesh.vertices.clear();
  if (mRenderData.rdDrawModelCoordArrows) {
    ///
    mQuatArrowMesh = mArrowModel.getVertexData();
    std::for_each(mQuatArrowMesh.vertices.begin(), mQuatArrowMesh.vertices.end(),
      [=](auto &n){
        glm::quat position = glm::quat(0.0f, n.position.x, n.position.y, n.position.z);
        glm::quat newPosition =
          mQuatModelOrientation * position * mQuatModelOrientConjugate;
        n.position.x = newPosition.x;
        n.position.y = newPosition.y;
        n.position.z = newPosition.z;
        n.position += mQuatModelPos;

    });
    mAllMeshes->vertices.insert(mAllMeshes->vertices.end(),
      mQuatArrowMesh.vertices.begin(), mQuatArrowMesh.vertices.end());

    mSphereArrowMesh = mArrowModel.getVertexData();
    std::for_each(mSphereArrowMesh.vertices.begin(), mSphereArrowMesh.vertices.end(),
      [=](auto &n){
        glm::quat position = glm::quat(0.0f, n.position.x, n.position.y, n.position.z);
        glm::quat newPosition =
          mSphereModelOrientation * position * mSphereModelOrientConjugate;
        n.position.x = newPosition.x;
        n.position.y = newPosition.y;
        n.position.z = newPosition.z;
        n.position += mSphereModelPos;

    });
    mAllMeshes->vertices.insert(mAllMeshes->vertices.end(),
      mSphereArrowMesh.vertices.begin(), mSphereArrowMesh.vertices.end());
  }

  mSpringLineMesh.vertices.clear();
  VkVertex anchor1Vertex;
  anchor1Vertex.position = mRigidBodyWorld.getRigidBody(NUMBER_OF_BRIDGE_POINTS * 3 - 1)->getPosition();
  anchor1Vertex.color = glm::vec3(0.0f, 1.0f, 0.0f);
  VkVertex cableEndVertex;
  cableEndVertex.position = mBoxModel->getRigidBody()->getPosition();
  cableEndVertex.color = glm::vec3(1.0f);
  mSpringLineMesh.vertices.push_back(anchor1Vertex);
  mSpringLineMesh.vertices.push_back(cableEndVertex);

  anchor1Vertex.position = mRigidBodyWorld.getRigidBody(NUMBER_OF_BRIDGE_POINTS * 3)->getPosition();
  anchor1Vertex.color = glm::vec3(0.0f, 1.0f, 0.0f);
  cableEndVertex.position = mSphereModel->getRigidBody()->getPosition();
  cableEndVertex.color = glm::vec3(1.0f);
  mSpringLineMesh.vertices.push_back(anchor1Vertex);
  mSpringLineMesh.vertices.push_back(cableEndVertex);

  /* anchor to plank */
  for (unsigned int i = 0; i < NUMBER_OF_BRIDGE_POINTS * 2; ++i) {
    VkVertex anchorVertex;
    anchorVertex.position = mRigidBodyWorld.getRigidBody(i)->getPosition();
    anchorVertex.color = glm::vec3(0.0f, 1.0f, 0.0f);
    mSpringLineMesh.vertices.push_back(anchorVertex);

    VkVertex plankVertex;
    plankVertex.position = mRigidBodyWorld.getRigidBody(i + NUMBER_OF_BRIDGE_POINTS * 2)->getPosition();
    plankVertex.color = glm::vec3(1.0f);
    mSpringLineMesh.vertices.push_back(plankVertex);
  }

  /* planks */
  for (unsigned int i = NUMBER_OF_BRIDGE_POINTS * 2; i < NUMBER_OF_BRIDGE_POINTS * 4; i += 2) {
    VkVertex plank1Vertex;
    plank1Vertex.position = mRigidBodyWorld.getRigidBody(i)->getPosition();
    plank1Vertex.color = glm::vec3(1.0f, 0.0f, 0.0f);
    mSpringLineMesh.vertices.push_back(plank1Vertex);

    VkVertex plank2Vertex;
    plank2Vertex.position = mRigidBodyWorld.getRigidBody(i + 1)->getPosition();
    plank2Vertex.color = glm::vec3(1.0f, 0.0f, 0.0f);
    mSpringLineMesh.vertices.push_back(plank2Vertex);
  }

  /* connectens between planks on every side */
  for (unsigned int i = NUMBER_OF_BRIDGE_POINTS * 2; i < NUMBER_OF_BRIDGE_POINTS * 4 - 2; ++i) {
    VkVertex plank1Vertex;
    plank1Vertex.position = mRigidBodyWorld.getRigidBody(i)->getPosition();
    plank1Vertex.color = glm::vec3(0.0f, 0.0f, 1.0f);
    mSpringLineMesh.vertices.push_back(plank1Vertex);

    VkVertex plank2Vertex;
    plank2Vertex.position = mRigidBodyWorld.getRigidBody(i + 2)->getPosition();
    plank2Vertex.color = glm::vec3(0.0f, 0.0f, 1.0f);
    mSpringLineMesh.vertices.push_back(plank2Vertex);
  }

  mAllMeshes->vertices.insert(mAllMeshes->vertices.end(),
    mSpringLineMesh.vertices.begin(), mSpringLineMesh.vertices.end());

  /* draw box model */
  *mQuatModelMesh = mBoxModel->getVertexData();
  mRenderData.rdTriangleCount = mQuatModelMesh->vertices.size() / 3;
  std::for_each(mQuatModelMesh->vertices.begin(), mQuatModelMesh->vertices.end(),
    [=](auto &n){
      glm::quat position = glm::quat(0.0f, n.position.x, n.position.y, n.position.z);
      glm::quat newPosition =
        mQuatModelOrientation * position * mQuatModelOrientConjugate;
      n.position.x = newPosition.x;
      n.position.y = newPosition.y;
      n.position.z = newPosition.z;
      n.position  += mQuatModelPos;

      glm::mat3 normalMat = glm::transpose(glm::mat3_cast(mQuatModelOrientConjugate));
      n.normal = glm::normalize(normalMat * n.normal);
  });
  mAllMeshes->vertices.insert(mAllMeshes->vertices.end(),
    mQuatModelMesh->vertices.begin(), mQuatModelMesh->vertices.end());

  /* draw sphere */
  *mSphereModelMesh = mSphereModel->getVertexData();
  mRenderData.rdFlatTriangleCount = mSphereModelMesh->vertices.size() / 3;
  std::for_each(mSphereModelMesh->vertices.begin(), mSphereModelMesh->vertices.end(),
    [=](auto &n){
      glm::quat position = glm::quat(0.0f, n.position.x, n.position.y, n.position.z);
      glm::quat newPosition =
        mSphereModelOrientation * position * mSphereModelOrientConjugate;
      n.position.x = newPosition.x;
      n.position.y = newPosition.y;
      n.position.z = newPosition.z;
      n.position  += mSphereModelPos;

      glm::mat3 normalMat = glm::transpose(glm::mat3_cast(mSphereModelOrientConjugate));
      n.normal = glm::normalize(normalMat * n.normal);
  });
  mAllMeshes->vertices.insert(mAllMeshes->vertices.end(),
    mSphereModelMesh->vertices.begin(), mSphereModelMesh->vertices.end());

  /* count line start in vertex buffer */
  mLineIndexCount = mCoordArrowsMesh.vertices.size() +
    mQuatArrowMesh.vertices.size() + mSphereArrowMesh.vertices.size() + mSpringLineMesh.vertices.size();

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
  VertexBuffer::uploadData(mRenderData, *mAllMeshes);
  mRenderData.rdUploadToVBOTime = mUploadToVBOTimer.stop();

  /* the rendering itself happens here */
  vkCmdBeginRenderPass(mRenderData.rdCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

  /* required for dynamic viewport */
  vkCmdSetViewport(mRenderData.rdCommandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(mRenderData.rdCommandBuffer, 0, 1, &scissor);

  /* vertex buffer */
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(mRenderData.rdCommandBuffer, 0, 1, &mRenderData.rdVertexBuffer, &offset);

  vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdPipelineLayout, 0, 1, &mRenderData.rdTextureDescriptorSet, 0, nullptr);
  vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdPipelineLayout, 1, 1, &mRenderData.rdUBODescriptorSet, 0, nullptr);

  /* draw lines first */
  if (mLineIndexCount > 0) {
    vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdLinePipeline);
    vkCmdSetLineWidth(mRenderData.rdCommandBuffer, 3.0f);
    vkCmdDraw(mRenderData.rdCommandBuffer, mLineIndexCount, 1, 0, 0);
  }

  /* draw models last, after lines */

  /* draw textured models */
  vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdBasicPipeline);
  vkCmdDraw(mRenderData.rdCommandBuffer, mRenderData.rdTriangleCount * 3, 1, mLineIndexCount, 0);

  /* draw flat models */
  vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdFlatPipeline);
  vkCmdDraw(mRenderData.rdCommandBuffer, mRenderData.rdFlatTriangleCount * 3, 1, mLineIndexCount + mRenderData.rdTriangleCount * 3, 0);

  // imgui overlay
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
  vmaMapMemory(mRenderData.rdAllocator, mRenderData.rdUboBufferAlloc, &data);
  memcpy(data, &mMatrices, static_cast<uint32_t>(sizeof(VkUploadMatrices)));
  vmaUnmapMemory(mRenderData.rdAllocator, mRenderData.rdUboBufferAlloc);
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

  return true;
}
