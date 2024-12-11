#include "pve/pve_window.hpp"

#include <stdexcept>

namespace pve {

PveWindow::PveWindow(int w, int h, std::string name) : width{w}, height{h}, windowName{name} {
    initWindow();
}

PveWindow::~PveWindow() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void PveWindow::initWindow() {
    // Initialize the GLFW library
    glfwInit();
    // GLFW was originally designed to create an OpenGL context when a window is created.
    // With GLFW_NO_API I'm telling GLFW not to create an OpenGL context since we're using Vulkan.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Disable our window from being resized after creation
    // We'll handle window resizes in a special way
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    // Initialize the window pointer
    // The 4th parameter is if we want to make a full screen window. For windowed mode, use a nullptr.
    // We can ignore the 5th parameter since it's for OpenGL context
    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizedCallback);
}

void PveWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface");
    }
}

void PveWindow::framebufferResizedCallback(GLFWwindow *window, int width, int height) {
    auto pveWindow = reinterpret_cast<PveWindow *>(glfwGetWindowUserPointer(window));
    pveWindow->framebufferResized = true;
    pveWindow->width = width;
    pveWindow->height = height;
}

}  // namespace pve