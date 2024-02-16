#include "commandPool.h"
#include <stdexcept>

namespace framework
{
    CommandPool::CommandPool(const std::shared_ptr<LogicalDevice> &lDevice, const VkSurfaceKHR &surface)
    {
        if (lDevice == nullptr)
        {
            throw std::runtime_error("[CommandPool] Logical device instance null");
        }

        this->lDevice = lDevice;

        // Retrieve the queue families
        QueueFamilyIndices indices = lDevice->findQueueFamilies(surface);

        // Create the command pool
        VkCommandPoolCreateInfo poolInfo{};

        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

        if (vkCreateCommandPool(lDevice->getDevice(), &poolInfo, nullptr, &pool) != VK_SUCCESS)
        {
            throw std::runtime_error("[CommandBuffer] Error creating the command pool");
        }
    }

    CommandPool::~CommandPool()
    {
        if (pool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(lDevice->getDevice(), pool, nullptr);
        }
    }
}