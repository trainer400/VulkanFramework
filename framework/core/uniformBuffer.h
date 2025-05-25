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
        uint32_t binding_index = 0;
        VkShaderStageFlags stage_flags = VK_SHADER_STAGE_VERTEX_BIT;
    };

    template <typename T>
    class UniformBuffer : public DescriptorElement
    {
    public:
        UniformBuffer(const std::shared_ptr<LogicalDevice> &l_device, const UniformBufferConfiguration &config);
        ~UniformBuffer();

        // Set the data
        void setData(const T &data);

        // Getters
        const VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding() override;
        const VkDescriptorPoolSize getPoolSize() override;
        const VkWriteDescriptorSet getWriteDescriptorSet() override;
        inline const VkBuffer &getUniformBuffer() { return uniform_buffer; }

    private:
        /**
         * @brief Allocates a buffer of memory depending on the passed parameters
         */
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory);

        uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);

        std::shared_ptr<LogicalDevice> l_device;

        UniformBufferConfiguration config;

        // UBO
        VkBuffer uniform_buffer = VK_NULL_HANDLE;
        VkDeviceMemory uniform_buffer_memory = VK_NULL_HANDLE;
        VkDescriptorBufferInfo buffer_info{};
        void *mapped_memory;
    };

    template <typename T>
    UniformBuffer<T>::UniformBuffer(const std::shared_ptr<LogicalDevice> &l_device, const UniformBufferConfiguration &config)
        : DescriptorElement(config.binding_index), config(config)
    {
        if (l_device == nullptr)
        {
            throw std::runtime_error("[UniformBuffer] Null logical device instance");
        }

        this->l_device = l_device;

        VkDeviceSize buffer_size = sizeof(T);

        createBuffer(buffer_size,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uniform_buffer, uniform_buffer_memory);

        // Persistent memory mapping
        vkMapMemory(l_device->getDevice(), uniform_buffer_memory, 0, buffer_size, 0, &mapped_memory);

        buffer_info.buffer = uniform_buffer;
        buffer_info.offset = 0;
        buffer_info.range = sizeof(T);
    }

    template <typename T>
    UniformBuffer<T>::~UniformBuffer()
    {
        if (uniform_buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(l_device->getDevice(), uniform_buffer, nullptr);
        }

        if (uniform_buffer_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(l_device->getDevice(), uniform_buffer_memory, nullptr);
        }
    }

    template <typename T>
    void UniformBuffer<T>::setData(const T &data)
    {
        // Copy the data inside the shared memory
        memcpy(mapped_memory, &data, sizeof(T));
    }

    template <typename T>
    const VkDescriptorSetLayoutBinding UniformBuffer<T>::getDescriptorSetLayoutBinding()
    {
        VkDescriptorSetLayoutBinding ubo_layout_binding{};

        ubo_layout_binding.binding = config.binding_index;
        ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_layout_binding.descriptorCount = 1;
        ubo_layout_binding.stageFlags = config.stage_flags;
        ubo_layout_binding.pImmutableSamplers = nullptr;

        return ubo_layout_binding;
    }

    template <typename T>
    const VkDescriptorPoolSize UniformBuffer<T>::getPoolSize()
    {
        VkDescriptorPoolSize pool_size{};

        pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_size.descriptorCount = 1;

        return pool_size;
    }

    template <typename T>
    const VkWriteDescriptorSet UniformBuffer<T>::getWriteDescriptorSet()
    {
        VkWriteDescriptorSet descriptor_write{};

        // TODO dstBinding may be changed to have more than one uniform buffers
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstBinding = config.binding_index;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_info;
        descriptor_write.pImageInfo = nullptr;
        descriptor_write.pTexelBufferView = nullptr;

        return descriptor_write;
    }

    template <typename T>
    void UniformBuffer<T>::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory)
    {
        // Create vertex buffer
        VkBufferCreateInfo buffer_info{};

        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(l_device->getDevice(), &buffer_info, nullptr, &buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("[DrawableCollection] Impossible to create the buffer");
        }

        // Enumerate the memory requirements
        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(l_device->getDevice(), buffer, &memory_requirements);

        // Allocate the memory on GPU
        VkMemoryAllocateInfo alloc_info{};

        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = memory_requirements.size;
        alloc_info.memoryTypeIndex = findMemoryType(memory_requirements.memoryTypeBits, properties);

        if (vkAllocateMemory(l_device->getDevice(), &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS)
        {
            throw std::runtime_error("[DrawableCollection] Impossible to allocate the required memory on the GPU");
        }

        // Associate the buffer to the memory
        vkBindBufferMemory(l_device->getDevice(), buffer, buffer_memory, 0);
    }

    template <typename T>
    uint32_t UniformBuffer<T>::findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memory_properties;

        // Enumerate the memory properties
        vkGetPhysicalDeviceMemoryProperties(l_device->getPhysicalDevice()->getDevice(), &memory_properties);

        for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
        {
            if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("[DrawableCollection] Unable to find a suitable memory type");
    }

}