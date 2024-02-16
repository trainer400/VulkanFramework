#pragma once

#include <devices/logicalDevice.h>
#include <devices/physicalDevice.h>
#include <window/windowSurface.h>

#include <vulkan/vulkan.h>

#include <memory>

namespace framework
{
    class CommandPool
    {
    public:
        CommandPool(const std::shared_ptr<LogicalDevice> &lDevice, const VkSurfaceKHR &surface);
        ~CommandPool();

        // Getters
        const VkCommandPool &getCommandPool() { return pool; }

    private:
        std::shared_ptr<LogicalDevice> lDevice;

        VkCommandPool pool = VK_NULL_HANDLE;
    };
}