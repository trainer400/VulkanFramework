#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <memory>

namespace framework
{
    class Vulkan
    {
    public:
        /**
         * @brief Construct a new Vulkan object
         *
         * @param applicationName The name to communicate to the driver
         * @param engineName The engine name to communicate to the driver
         * @param addedExtensions Extensions to add to the engine
         */
        Vulkan(const char *applicationName, const char *engineName, const std::vector<const char *> &addedExtensions, bool enableLayers = false);
        ~Vulkan();

        // Getters
        const VkInstance &getInstance() { return instance; }

    private:
        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

        // Vulkan objects
        VkInstance instance = VK_NULL_HANDLE;
    };
}