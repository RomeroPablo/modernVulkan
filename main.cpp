import vulkan;

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <exception>
#include <iostream>

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, false);
  uint32_t width = 480, height = 480;
  GLFWwindow *window =
      glfwCreateWindow(width, height, "Modern Vulkan", nullptr, nullptr);
  try {
    Vulkan app;
    app.initWindow(window, width, height);
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  glfwDestroyWindow(window);
  glfwTerminate();
  return EXIT_SUCCESS;
}
