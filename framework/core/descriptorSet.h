#pragma once

#include <devices/logicalDevice.h>
#include <core/descriptorElement.h>

#include <vector>
#include <memory>

namespace framework
{
    class DescriptorSet
    {
    public:
        DescriptorSet(const std::shared_ptr<LogicalDevice> &l_device, const std::vector<std::shared_ptr<DescriptorElement>> &elements);
        ~DescriptorSet();

        // Getters
        inline const VkDescriptorPool &getDescriptorPool() { return pool; }
        inline const VkDescriptorSet &getDescriptorSet() { return descriptor_set; }
        inline const VkDescriptorSetLayout &getDescriptorSetLayout() { return descriptor_set_layout; }

    private:
        VkDescriptorPool pool = VK_NULL_HANDLE;
        VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;

        // Framework objects
        std::shared_ptr<LogicalDevice> l_device;
    };
}