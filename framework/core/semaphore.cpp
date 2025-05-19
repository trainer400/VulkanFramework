#include "semaphore.h"

#include <stdexcept>

namespace framework
{
    Semaphore::Semaphore(const std::shared_ptr<LogicalDevice> &l_device)
    {
        if (l_device == nullptr)
        {
            throw std::runtime_error("[Semaphore] Logical device has not been created yet");
        }
        this->l_device = l_device;

        // Specify the type
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // Create vulkan object
        if (vkCreateSemaphore(l_device->getDevice(), &info, nullptr, &semaphore))
        {
            throw std::runtime_error("[Semaphore] Impossible to create semaphore object");
        }
    }

    Semaphore::~Semaphore()
    {
        if (l_device != nullptr)
        {
            vkDestroySemaphore(l_device->getDevice(), semaphore, nullptr);
        }
    }
}