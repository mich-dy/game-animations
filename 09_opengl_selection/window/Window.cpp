#include "Window.h"
#include "Logger.h"

bool Window::init(unsigned int width, unsigned int height, std::string title) {
  if (!glfwInit()) {
    Logger::log(1, "%s: glfwInit() error\n", __FUNCTION__);
    return false;
  }

  /* set a "hint" for the NEXT window created*/
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  mWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

  if (!mWindow) {
    glfwTerminate();
    Logger::log(1, "%s error: Could not create window\n", __FUNCTION__);
    return false;
  }

  glfwMakeContextCurrent(mWindow);

  mRenderer = std::make_unique<OGLRenderer>(mWindow);

  glfwSetWindowUserPointer(mWindow, mRenderer.get());
  glfwSetWindowSizeCallback(mWindow, [](GLFWwindow *win, int width, int height) {
      auto renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(win));
      renderer->setSize(width, height);
    }
  );

  glfwSetKeyCallback(mWindow, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
      auto renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(win));
      renderer->handleKeyEvents(key, scancode, action, mods);
    }
  );

  glfwSetMouseButtonCallback(mWindow, [](GLFWwindow *win, int button, int action, int mods) {
      auto renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(win));
      renderer->handleMouseButtonEvents(button, action, mods);
    }
  );

  glfwSetCursorPosCallback(mWindow, [](GLFWwindow *win, double xpos, double ypos) {
      auto renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(win));
      renderer->handleMousePositionEvents(xpos, ypos);
    }
  );

  if (!mRenderer->init(width, height)) {
    glfwTerminate();
    Logger::log(1, "%s error: Could not init OpenGL\n", __FUNCTION__);
    return false;
  }

  Logger::log(1, "%s: Window with OpenGL 4.6 successfully initialized\n", __FUNCTION__);
  return true;
}


void Window::mainLoop() {
  /* force VSYNC */
  glfwSwapInterval(1);

  while (!glfwWindowShouldClose(mWindow)) {
    mRenderer->draw();

    /* swap buffers */
    glfwSwapBuffers(mWindow);

    /* poll events in a loop */
    glfwPollEvents();

  }
}

void Window::cleanup() {
  mRenderer->cleanup();

  glfwDestroyWindow(mWindow);
  glfwTerminate();
  Logger::log(1, "%s: Terminating Window\n", __FUNCTION__);
}
