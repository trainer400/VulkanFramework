#pragma once

#include <memory>
#include <devices/logicalDevice.h>

namespace framework
{
    class Fence
    {
    public:
        /**
         * @brief Construct a new Fence object
         *
         * @param l_device The logical device
         * @param signaled Whether the fence starts as signaled or not
         */
        Fence(std::shared_ptr<LogicalDevice> l_device, bool signaled);
        ~Fence();

        /**
         * @brief Waits for fence to be triggered
         */
        inline void waitFor(uint32_t fence_count) { vkWaitForFences(l_device->getDevice(), fence_count, &fence, VK_TRUE, UINT64_MAX); }

        /**
         * @brief Resets the status of the fence
         */
        inline void reset(uint32_t fence_count) { vkResetFences(l_device->getDevice(), fence_count, &fence); }

        // Getters
        inline const VkFence &getFence() { return fence; }

    private:
        // Framework objects
        std::shared_ptr<LogicalDevice> l_device;

        // Vulkan objects
        VkFence fence;
    };
}