/* Vulkan renderer */
#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
/* Vulkan also before GLFW */
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include "Timer.h"
#include "VkRenderData.h"
#include "Renderpass.h"
#include "Pipeline.h"
#include "PipelineLayout.h"
#include "Framebuffer.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "SyncObjects.h"
#include "Texture.h"
#include "UniformBuffer.h"
#include "UserInterface.h"
#include "Camera.h"

class VkRenderer {
  public:
    VkRenderer(GLFWwindow *window);

    bool init(unsigned int width, unsigned int height);
    void setSize(unsigned int width, unsigned int height);
    bool uploadData(VkMesh vertexData);
    bool draw();
    void handleKeyEvents(int key, int scancode, int action, int mods);
    void handleMouseButtonEvents(int button, int action, int mods);
    void handleMousePositionEvents(double xPos, double yPos);

    void toggleShader();
    void cleanup();

  private:
    VkRenderData mRenderData{};

    UserInterface mUserInterface{};
    Camera mCamera{};

    bool mMouseLock = false;
    int mMouseXPos = 0;
    int mMouseYPos = 0;

    double mLastTickTime = 0.0;

    void handleMovementKeys();
    int mCameraForward = 0;
    int mCameraStrafe = 0;
    int mCameraUpDown = 0;

    Timer mFrameTimer{};
    Timer mMatrixGenerateTimer{};
    Timer mUploadToUBOTimer{};
    Timer mUIGenerateTimer{};
    Timer mUIDrawTimer{};

    VkSurfaceKHR mSurface = VK_NULL_HANDLE;

    VkDeviceSize mMinUniformBufferOffsetAlignment = 0;

    VkBuffer mVertexBuffer = VK_NULL_HANDLE;
    VmaAllocation mVertexBufferAlloc = nullptr;

    VkUploadMatrices mMatrices{};

    bool deviceInit();
    bool getQueue();
    bool createDepthBuffer();
    bool createUBO();
    bool createSwapchain();
    bool createRenderPass();
    bool createPipelineLayout();
    bool createBasicPipeline();
    bool createChangedPipeline();
    bool createFramebuffer();
    bool createCommandPool();
    bool createCommandBuffer();
    bool createSyncObjects();
    bool loadTexture();
    bool initUserInterface();

    bool initVma();

    bool recreateSwapchain();
};
