#pragma once

#include <devices/physicalDevice.h>
#include <devices/logicalDevice.h>
#include <window/windowSurface.h>
#include <core/commandPool.h>

#include <vulkan/vulkan.h>

#include <memory>

namespace framework
{
    class CommandBuffer
    {
    public:
        CommandBuffer(const std::shared_ptr<LogicalDevice> &lDevice, const VkCommandPool &pool);

        /**
         * @brief Starts the recording of commands to store into the command buffer
         */
        void beginRecording();

        /**
         * @brief Stops the recording of commands into the command buffer
         */
        void stopRecording();

        // Getters
        const VkCommandBuffer &getCommandBuffer() { return buffer; }

    private:
        std::shared_ptr<LogicalDevice> lDevice;

        VkCommandBuffer buffer = VK_NULL_HANDLE;
    };
}