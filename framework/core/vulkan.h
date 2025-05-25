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
         * @param application_name The name to communicate to the driver
         * @param engine_name The engine name to communicate to the driver
         * @param added_extensions Extensions to add to the engine
         */
        Vulkan(const char *application_name, const char *engine_name, const std::vector<const char *> &added_extensions, bool enable_layers = false);
        ~Vulkan();

        // Getters
        const VkInstance &getInstance() { return instance; }

    private:
        const std::vector<const char *> validation_layers = {"VK_LAYER_KHRONOS_validation"};

        // Vulkan objects
        VkInstance instance = VK_NULL_HANDLE;
    };
}