#include "commandPool.h"
#include <stdexcept>

namespace framework
{
    CommandPool::CommandPool(const std::shared_ptr<LogicalDevice> &l_device, const VkSurfaceKHR &surface)
    {
        if (l_device == nullptr)
        {
            throw std::runtime_error("[CommandPool] Logical device instance null");
        }

        this->l_device = l_device;

        // Retrieve the queue families
        QueueFamilyIndices indices = l_device->findQueueFamilies(surface);

        // Create the command pool
        VkCommandPoolCreateInfo pool_info{};

        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = indices.graphicsFamily.value();

        if (vkCreateCommandPool(l_device->getDevice(), &pool_info, nullptr, &pool) != VK_SUCCESS)
        {
            throw std::runtime_error("[CommandBuffer] Error creating the command pool");
        }
    }

    CommandPool::~CommandPool()
    {
        if (pool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(l_device->getDevice(), pool, nullptr);
        }
    }
}