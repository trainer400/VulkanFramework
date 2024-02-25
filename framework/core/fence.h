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
         * @param lDevice The logical device
         * @param signaled Whether the fence starts as signaled or not
         */
        Fence(std::shared_ptr<LogicalDevice> lDevice, bool signaled);
        ~Fence();

        /**
         * @brief Waits for fence to be triggered
         */
        inline void waitFor(uint32_t fenceCount) { vkWaitForFences(lDevice->getDevice(), fenceCount, &fence, VK_TRUE, UINT64_MAX); }

        /**
         * @brief Resets the status of the fence
         */
        inline void reset(uint32_t fenceCount) { vkResetFences(lDevice->getDevice(), fenceCount, &fence); }

        // Getters
        inline const VkFence &getFence() { return fence; }

    private:
        // Framework objects
        std::shared_ptr<LogicalDevice> lDevice;

        // Vulkan objects
        VkFence fence;
    };
}