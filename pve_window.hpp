// Ensure that the contents of a given header file are not copied, more
// than once, into any single file to prevent duplicate definitions
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace pve
{

    class PveWindow
    {

    public:
        PveWindow(int w, int h, std::string name);
        ~PveWindow();

        // resource creation happens when we initialize our variables and cleanups are performed
        // by the destructors. We don't want to accidentally copy a PVE window and then have 
        // two pointers to the GLFW window. if this happens, when one of these object's destructors
        // are called, the shared GLFW window would be terminated and we'd be left with a dangling pointer
        PveWindow(const PveWindow &) = delete;
        PveWindow &operator = (const PveWindow &) = delete;

        bool shouldClose() { return glfwWindowShouldClose(window); }
        VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);
        bool wasWindowResized() { return frameBufferResized; }
        void resetWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

    private:
        static void frameBufferResizeCallback(GLFWwindow *window, int width, int height);
        void initWindow();
        int width;
        int height;
        bool frameBufferResized = false;

        std::string windowName;
        GLFWwindow *window;
    };

}