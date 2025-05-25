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
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;
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

        /**
         * @brief Waits for the device to be idle
         */
        inline void waitIdle() { vkDeviceWaitIdle(device); }

        // Getters
        inline const VkDevice &getDevice() { return device; }
        inline const VkQueue &getGraphicsQueue() { return graphics_queue; }
        inline const VkQueue &getPresentQueue() { return present_queue; }
        inline const std::unique_ptr<PhysicalDevice> &getPhysicalDevice() { return p_device; }

    private:
        std::unique_ptr<PhysicalDevice> p_device;

        VkDevice device = VK_NULL_HANDLE;
        VkQueue graphics_queue = VK_NULL_HANDLE;
        VkQueue present_queue = VK_NULL_HANDLE;
    };
}