/* Vulkan renderer */
#pragma once

#include <vector>
#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

/* include Vulkan before GLFW */
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include "Timer.h"
#include "Renderpass.h"
#include "Pipeline.h"
#include "PipelineLayout.h"
#include "Framebuffer.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "SyncObjects.h"
#include "Texture.h"
#include "UniformBuffer.h"
#include "VertexBuffer.h"
#include "UserInterface.h"
#include "Camera.h"
#include "BoxModel.h"
#include "CoordArrowsModel.h"
#include "ArrowModel.h"
#include "SphereModel.h"

#include "RigidBody.h"
#include "RigidBodyWorld.h"
#include "ForceRegistry.h"
#include "WindForce.h"

#include "VkRenderData.h"

class VkRenderer {
  public:
    VkRenderer(GLFWwindow *window);

    bool init(unsigned int width, unsigned int height);
    void setSize(unsigned int width, unsigned int height);

    bool initModel();

    bool draw(const float deltaTime);
    void handleKeyEvents(int key, int scancode, int action, int mods);
    void handleMouseButtonEvents(int button, int action, int mods);
    void handleMousePositionEvents(double xPos, double yPos);

    void cleanup();

  private:
    VkRenderData mRenderData{};

    VkVertexBufferData mLineVertexBuffer{};
    VkVertexBufferData mPolygonVertexBuffer{};

    UserInterface mUserInterface{};
    Camera mCamera{};

    CoordArrowsModel mCoordArrowsModel{};
    VkLineMesh mCoordArrowsMesh{};

    ArrowModel mArrowModel{};
    VkLineMesh mQuatArrowMesh{};
    VkLineMesh mSphereArrowMesh{};
    VkLineMesh mSpringLineMesh{};

    /* TODO: configure max contacts */
    const unsigned int NUMBER_OF_BRIDGE_POINTS = 5;

    RigidBodyWorld mRigidBodyWorld = RigidBodyWorld(NUMBER_OF_BRIDGE_POINTS * 10, NUMBER_OF_BRIDGE_POINTS * 20);

    ForceRegistry mForceRegistry{};
    std::shared_ptr<WindForce> mWindForce = nullptr;

    std::shared_ptr<BoxModel> mBoxModel = nullptr;
    std::unique_ptr<VkMesh> mQuatModelMesh = nullptr;

    std::shared_ptr<SphereModel> mSphereModel = nullptr;
    std::shared_ptr<VkMesh> mSphereModelMesh = nullptr;

    std::unique_ptr<VkMesh> mAllMeshes = nullptr;
    std::unique_ptr<VkLineMesh> mLineMeshes = nullptr;

    unsigned int mLineIndexCount = 0;

    glm::mat4 mRotYMat = glm::mat4(1.0f);
    glm::mat4 mRotZMat = glm::mat4(1.0f);

    /* initial position */
    glm::vec3 mSpring1AnchorPos = glm::vec3(1.0f, 0.0f, -1.0f);
    glm::vec3 mSpring2AnchorPos = glm::vec3(1.0f, 0.0f, 1.0f);
    glm::vec3 mSpring3AnchorPos = glm::vec3(-1.0f, 0.0f, 1.0f);

    glm::vec3 mQuatModelInitialPos = glm::vec3(-0.5f, 0.0f, 3.0f);
    glm::vec3 mQuatModelPos = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::vec3 mRotXAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 mRotYAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 mRotZAxis = glm::vec3(0.0f, 0.0f, 1.0f);

    glm::quat mQuatModelOrientation = glm::quat();
    glm::quat mQuatModelOrientConjugate = glm::quat();

    glm::vec3 mSphereModelInitialPos = glm::vec3(-0.5f, 0.0f, -3.0f);
    glm::vec3 mSphereModelPos = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::quat mSphereModelOrientation = glm::quat();
    glm::quat mSphereModelOrientConjugate = glm::quat();

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
    Timer mUploadToVBOTimer{};
    Timer mUploadToUBOTimer{};
    Timer mUIGenerateTimer{};
    Timer mUIDrawTimer{};
    Timer mPhysicsTimer{};

    VkSurfaceKHR mSurface = VK_NULL_HANDLE;

    VkDeviceSize mMinUniformBufferOffsetAlignment = 0;

    VkUploadMatrices mMatrices{};

    bool deviceInit();
    bool getQueue();
    bool createDepthBuffer();
    bool createVBO();
    bool createLineVBO();
    bool createUBO();
    bool createSwapchain();
    bool createRenderPass();
    bool createPipelineLayout();
    bool createBasicPipeline();
    bool createLinePipeline();
    bool createFlatPipeline();
    bool createFramebuffer();
    bool createCommandPool();
    bool createCommandBuffer();
    bool createSyncObjects();
    bool loadTexture();
    bool initUserInterface();

    bool initVma();

    bool recreateSwapchain();
};
