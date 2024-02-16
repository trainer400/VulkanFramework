#pragma once

#include <vulkan/vulkan.h>

#include <devices/physicalDevice.h>
#include <devices/logicalDevice.h>
#include <core/descriptorElement.h>

#include <cstring>
#include <stdexcept>
#include <memory>

namespace framework
{
    struct UniformBufferConfiguration
    {
        uint32_t bindingIndex = 0;
        VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    };

    template <typename T>
    class UniformBuffer : public DescriptorElement
    {
    public:
        UniformBuffer(const std::shared_ptr<LogicalDevice> &lDevice, const UniformBufferConfiguration &config);
        ~UniformBuffer();

        // Set the data
        void setData(const T &data);

        // Getters
        const VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding() override;
        const VkDescriptorPoolSize getPoolSize() override;
        const VkWriteDescriptorSet getWriteDescriptorSet() override;
        inline const VkBuffer &getUniformBuffer() { return uniformBuffer; }

    private:
        /**
         * @brief Allocates a buffer of memory depending on the passed parameters
         */
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        std::shared_ptr<LogicalDevice> lDevice;

        UniformBufferConfiguration config;

        // UBO
        VkBuffer uniformBuffer = VK_NULL_HANDLE;
        VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
        VkDescriptorBufferInfo bufferInfo{};
        void *mappedMemory;
    };

    template <typename T>
    UniformBuffer<T>::UniformBuffer(const std::shared_ptr<LogicalDevice> &lDevice, const UniformBufferConfiguration &config)
        : DescriptorElement(config.bindingIndex), config(config)
    {
        if (lDevice == nullptr)
        {
            throw std::runtime_error("[UniformBuffer] Null logical device instance");
        }

        this->lDevice = lDevice;

        VkDeviceSize bufferSize = sizeof(T);

        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uniformBuffer, uniformBufferMemory);

        // Persistent memory mapping
        vkMapMemory(lDevice->getDevice(), uniformBufferMemory, 0, bufferSize, 0, &mappedMemory);

        bufferInfo.buffer = uniformBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(T);
    }

    template <typename T>
    UniformBuffer<T>::~UniformBuffer()
    {
        if (uniformBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(lDevice->getDevice(), uniformBuffer, nullptr);
        }

        if (uniformBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(lDevice->getDevice(), uniformBufferMemory, nullptr);
        }
    }

    template <typename T>
    void UniformBuffer<T>::setData(const T &data)
    {
        // Copy the data inside the shared memory
        memcpy(mappedMemory, &data, sizeof(T));
    }

    template <typename T>
    const VkDescriptorSetLayoutBinding UniformBuffer<T>::getDescriptorSetLayoutBinding()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};

        uboLayoutBinding.binding = config.bindingIndex;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = config.stageFlags;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        return uboLayoutBinding;
    }

    template <typename T>
    const VkDescriptorPoolSize UniformBuffer<T>::getPoolSize()
    {
        VkDescriptorPoolSize poolSize{};

        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = 1;

        return poolSize;
    }

    template <typename T>
    const VkWriteDescriptorSet UniformBuffer<T>::getWriteDescriptorSet()
    {
        VkWriteDescriptorSet descriptorWrite{};

        // TODO dstBinding may be changed to have more than one uniform buffers
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstBinding = config.bindingIndex;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;

        return descriptorWrite;
    }

    template <typename T>
    void UniformBuffer<T>::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
    {
        // Create vertex buffer
        VkBufferCreateInfo bufferInfo{};

        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(lDevice->getDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("[DrawableCollection] Impossible to create the buffer");
        }

        // Enumerate the memory requirements
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(lDevice->getDevice(), buffer, &memRequirements);

        // Allocate the memory on GPU
        VkMemoryAllocateInfo allocInfo{};

        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(lDevice->getDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("[DrawableCollection] Impossible to allocate the required memory on the GPU");
        }

        // Associate the buffer to the memory
        vkBindBufferMemory(lDevice->getDevice(), buffer, bufferMemory, 0);
    }

    template <typename T>
    uint32_t UniformBuffer<T>::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;

        // Enumerate the memory properties
        vkGetPhysicalDeviceMemoryProperties(lDevice->getPhysicalDevice()->getDevice(), &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("[DrawableCollection] Unable to find a suitable memory type");
    }

}