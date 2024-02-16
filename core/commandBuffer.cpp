#include "commandBuffer.h"
#include <stdexcept>

namespace framework
{
    CommandBuffer::CommandBuffer(const std::shared_ptr<LogicalDevice> &lDevice, const VkCommandPool &pool)
    {
        if (lDevice == nullptr)
        {
            throw std::runtime_error("[CommandBuffer] Device instance null");
        }

        if (pool == nullptr)
        {
            throw std::runtime_error("[CommandBuffer] Pool instance null");
        }

        this->lDevice = lDevice;

        // Create the command buffer using the created command pool
        VkCommandBufferAllocateInfo allocInfo{};

        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(lDevice->getDevice(), &allocInfo, &buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("[CommandBuffer] Impossible to allocate a command buffer");
        }
    }

    void CommandBuffer::beginRecording()
    {
        VkCommandBufferBeginInfo beginInfo{};

        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("[CommandBuffer] Impossible to begin recording");
        }
    }

    void CommandBuffer::stopRecording()
    {
        if (vkEndCommandBuffer(buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("[CommandBuffer] Error ending the command buffer recording");
        }
    }
}