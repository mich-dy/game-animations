#pragma once
#include <string>
#include <memory>
#include <chrono>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "VkRenderer.h"

class Window {
  public:
    bool init(unsigned int width, unsigned int height, std::string title);
    void mainLoop();
    void cleanup();

  private:
    GLFWwindow *mWindow = nullptr;

    std::unique_ptr<VkRenderer> mRenderer;
    std::chrono::time_point<std::chrono::steady_clock> mStartTime;
};
