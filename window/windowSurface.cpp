#include "windowSurface.h"
#include <stdexcept>

namespace framework
{
    WindowSurface::WindowSurface(VkInstance instance, GLFWwindow *window)
    {
        // Null check
        if (instance == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[WindowSurface] Null vulkan instance");
        }
        if (window == nullptr)
        {
            throw std::runtime_error("[WindowSurface] Null window instance");
        }

        // Set the parameters
        this->instance = instance;
        this->window = window;

        // Create the surface
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("[WindowSurface] Error creating window surface");
        }
    }

    WindowSurface::~WindowSurface()
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
}