#include "commandBuffer.h"
#include <stdexcept>

namespace framework
{
    CommandBuffer::CommandBuffer(const std::shared_ptr<LogicalDevice> &l_device, const VkCommandPool &pool)
    {
        if (l_device == nullptr)
        {
            throw std::runtime_error("[CommandBuffer] Device instance null");
        }

        if (pool == nullptr)
        {
            throw std::runtime_error("[CommandBuffer] Pool instance null");
        }

        this->l_device = l_device;

        // Create the command buffer using the created command pool
        VkCommandBufferAllocateInfo alloc_info{};

        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(l_device->getDevice(), &alloc_info, &buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("[CommandBuffer] Impossible to allocate a command buffer");
        }
    }

    void CommandBuffer::beginRecording()
    {
        VkCommandBufferBeginInfo begin_info{};

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(buffer, &begin_info) != VK_SUCCESS)
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