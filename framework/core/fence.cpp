#include "fence.h"
#include <stdexcept>

using namespace std;

namespace framework
{
    Fence::Fence(shared_ptr<LogicalDevice> lDevice, bool signaled)
    {
        if (lDevice == nullptr)
        {
            throw runtime_error("[Fence] Null logical device instance");
        }

        this->lDevice = lDevice;

        // Create the vulkan fence object
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        // Create the fence with the first state signaled depending on the bool value
        fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

        if (vkCreateFence(lDevice->getDevice(), &fenceInfo, nullptr, &fence) != VK_SUCCESS)
        {
            throw std::runtime_error("[Vulkan] Impossible to create sync objects");
        }
    }

    Fence::~Fence()
    {
        if (lDevice != nullptr)
        {
            vkDestroyFence(lDevice->getDevice(), fence, nullptr);
        }
    }
}