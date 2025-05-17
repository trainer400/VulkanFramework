#pragma once

#include <stdint.h>

#include <vulkan/vulkan.h>

namespace framework
{
    class DescriptorElement
    {
    public:
        DescriptorElement(uint32_t binding_index) : binding_index(binding_index) {}

        // Getters
        virtual const VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding() = 0;
        virtual const VkDescriptorPoolSize getPoolSize() = 0;
        virtual const VkWriteDescriptorSet getWriteDescriptorSet() = 0;

    protected:
        uint32_t binding_index;
    };
}