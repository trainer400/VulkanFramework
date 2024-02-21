#include "semaphore.h"

#include <stdexcept>

namespace framework
{
    Semaphore::Semaphore(const std::shared_ptr<LogicalDevice> &lDevice)
    {
        if (lDevice == nullptr)
        {
            throw std::runtime_error("[Semaphore] Logical device has not been created yet");
        }
        this->lDevice = lDevice;

        // Specify the type
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // Create vulkan object
        if (vkCreateSemaphore(lDevice->getDevice(), &info, nullptr, &semaphore))
        {
            throw std::runtime_error("[Semaphore] Impossible to create semaphore object");
        }
    }

    Semaphore::~Semaphore()
    {
        if (lDevice != nullptr)
        {
            vkDestroySemaphore(lDevice->getDevice(), semaphore, nullptr);
        }
    }
}