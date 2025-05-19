#pragma once

#include <devices/logicalDevice.h>

#include <memory>

namespace framework
{
    class Semaphore
    {
    public:
        Semaphore(const std::shared_ptr<LogicalDevice> &l_device);
        ~Semaphore();

        inline const VkSemaphore &getSemaphore() { return semaphore; }

    private:
        VkSemaphore semaphore;

        // Framework objects
        std::shared_ptr<LogicalDevice> l_device;
    };
}