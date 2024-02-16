#pragma once

#include <vulkan/vulkan.h>
#include <devices/physicalDevice.h>
#include <window/windowSurface.h>
#include <optional>
#include <memory>

namespace framework
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
    };

    class LogicalDevice
    {
    public:
        LogicalDevice(std::unique_ptr<PhysicalDevice> p, const VkSurfaceKHR &surface);
        ~LogicalDevice() { vkDestroyDevice(device, nullptr); }

        /**
         * @brief Finds all the queues for the selected physical device and surface
         *
         * TODO: make this method static
         *
         * @return QueueFamilyIndices struct of indices references
         */
        QueueFamilyIndices findQueueFamilies(const VkSurfaceKHR &surface);

        // Gettes
        inline const VkDevice &getDevice() { return device; }
        inline const VkQueue &getGraphicsQueue() { return graphicsQueue; }
        inline const VkQueue &getPresentQueue() { return presentQueue; }
        inline const std::unique_ptr<PhysicalDevice> &getPhysicalDevice() { return pDevice; }

    private:
        std::unique_ptr<PhysicalDevice> pDevice;

        VkDevice device = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
    };
}