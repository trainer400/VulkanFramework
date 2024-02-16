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
        DescriptorSet(const std::shared_ptr<LogicalDevice> &lDevice, const std::vector<std::shared_ptr<DescriptorElement>> &elements);
        ~DescriptorSet();

        // Getters
        inline const VkDescriptorPool &getDescriptorPool() { return pool; }
        inline const VkDescriptorSet &getDescriptorSet() { return descriptorSet; }
        inline const VkDescriptorSetLayout &getDescriptorSetLayout() { return descriptorSetLayout; }

    private:
        VkDescriptorPool pool = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

        // Framework objects
        std::shared_ptr<LogicalDevice> lDevice;
    };
}