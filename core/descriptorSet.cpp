#include "descriptorSet.h"

#include <stdexcept>

namespace framework
{
    DescriptorSet::DescriptorSet(const std::shared_ptr<LogicalDevice> &lDevice, const std::vector<std::shared_ptr<DescriptorElement>> &elements)
    {
        if (lDevice == nullptr)
        {
            throw std::runtime_error("[DescriptorSet] Null logical device instance");
        }

        if (elements.size() == 0)
        {
            throw std::runtime_error("[DescriptorSet] Void descriptor elements");
        }

        // TODO add check that all the bindings are progressive and consistent and not overlapping
        this->lDevice = lDevice;

        // Copy all the config data structures
        std::vector<VkDescriptorPoolSize> poolSizes{elements.size()};
        std::vector<VkDescriptorSetLayoutBinding> bindings{elements.size()};
        std::vector<VkWriteDescriptorSet> descriptorWrites{elements.size()};

        for (int i = 0; i < elements.size(); i++)
        {
            poolSizes[i] = elements[i]->getPoolSize();
            bindings[i] = elements[i]->getDescriptorSetLayoutBinding();
            descriptorWrites[i] = elements[i]->getWriteDescriptorSet();
        }

        // Configure the descriptor set
        VkDescriptorPoolCreateInfo poolInfo{};

        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(lDevice->getDevice(), &poolInfo, nullptr, &pool) != VK_SUCCESS)
        {
            throw std::runtime_error("[DescriptorPool] Impossible to create descriptor pool");
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};

        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(lDevice->getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("[UniformBuffer] Error creating descriptor set layout");
        }

        // Create the descriptor set
        VkDescriptorSetAllocateInfo allocInfo{};

        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        if (vkAllocateDescriptorSets(lDevice->getDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS)
        {
            throw std::runtime_error("[UniformBuffer] Error creating the descriptor set");
        }

        // Update the descriptor set inside the writes structs
        for (int i = 0; i < descriptorWrites.size(); i++)
        {
            descriptorWrites[i].dstSet = descriptorSet;
        }

        vkUpdateDescriptorSets(lDevice->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    DescriptorSet::~DescriptorSet()
    {
        if (pool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(lDevice->getDevice(), pool, nullptr);
        }

        if (descriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(lDevice->getDevice(), descriptorSetLayout, nullptr);
        }
    }

}