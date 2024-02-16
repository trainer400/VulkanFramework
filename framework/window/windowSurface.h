#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
namespace framework
{
    class WindowSurface
    {
    public:
        /**
         * @brief Construct a new Window Surface object
         *
         * @param instance Vulkan instance
         * @param window GLFW pre-created window
         */
        WindowSurface(VkInstance instance, GLFWwindow *window);
        ~WindowSurface();

        // Getter
        inline const VkSurfaceKHR &getSurface() { return surface; }

    private:
        // Window surface to be created
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkInstance instance = VK_NULL_HANDLE;
        GLFWwindow *window;
    };
}