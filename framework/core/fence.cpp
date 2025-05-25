#include "fence.h"
#include <stdexcept>

using namespace std;

namespace framework
{
    Fence::Fence(shared_ptr<LogicalDevice> l_device, bool signaled)
    {
        if (l_device == nullptr)
        {
            throw runtime_error("[Fence] Null logical device instance");
        }

        this->l_device = l_device;

        // Create the vulkan fence object
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        // Create the fence with the first state signaled depending on the bool value
        fence_info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

        if (vkCreateFence(l_device->getDevice(), &fence_info, nullptr, &fence) != VK_SUCCESS)
        {
            throw std::runtime_error("[Vulkan] Impossible to create sync objects");
        }
    }

    Fence::~Fence()
    {
        if (l_device != nullptr)
        {
            vkDestroyFence(l_device->getDevice(), fence, nullptr);
        }
    }
}