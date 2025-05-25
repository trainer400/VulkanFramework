#pragma once

#include <vulkan/vulkan.h>

#include <core/drawableElement.h>
#include <core/commandPool.h>
#include <core/commandBuffer.h>
#include <core/shader.h>
#include <core/descriptorSet.h>
#include <core/commandBuffer.h>
#include <core/vertexAttributes.h>
#include <devices/logicalDevice.h>
#include <devices/physicalDevice.h>

#include <vector>
#include <memory>

namespace framework
{
    class DrawableCollection
    {
    public:
        DrawableCollection(const std::shared_ptr<LogicalDevice> &l_device, std::unique_ptr<DescriptorSet> descriptor, const VkCommandPool &pool, const std::vector<std::shared_ptr<Shader>> &shaders);
        ~DrawableCollection();

        /**
         * @brief Adds the drawable element inside the list unless the buffer has already been allocated
         * @throws Runtime Exception if the buffer has already been allocated
         */
        void addElement(const std::shared_ptr<DrawableElement> &element);

        /**
         * @brief Allocates the buffer inside the GPU memory if not already done
         * @throws Runtime Exception if the buffer is already allocated
         */
        void allocate();

        /**
         * @brief Updates the elements inside the vertex and indices vectors and transfers the modifications
         * into the GPU memory
         */
        void updateElements();

        // Getters
        VkVertexInputBindingDescription getBindingDescription();
        std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
        const VkBuffer &getVertexBuffer() { return vertex_buffer; }
        const VkBuffer &getIndexBuffer() { return index_buffer; }
        uint32_t getVerticesNumber() { return vertices.size() / getAttributesFloatNumber(); }
        uint32_t getIndexSize() { return indices.size(); }
        uint32_t getNumberOfInstances() { return number_of_instances; }
        bool isAllocated() { return allocated; }
        const std::vector<std::shared_ptr<Shader>> &getShaders() { return shaders; }
        inline const VkDescriptorPool &getDescriptorPool() { return descriptor_set->getDescriptorPool(); }
        inline const VkDescriptorSet &getDescriptorSet() { return descriptor_set->getDescriptorSet(); }
        inline const VkDescriptorSetLayout &getDescriptorSetLayout() { return descriptor_set->getDescriptorSetLayout(); }
        inline bool hasDescriptorSet() { return descriptor_set != nullptr; }

        // Setters
        void setNumberOfInstances(uint32_t instances) { number_of_instances = instances; }

    private:
        /**
         * @brief Sums the number of floats per vertex
         */
        int getAttributesFloatNumber();

        /**
         * @brief Looks for the memory on the GPU that suits the passed parameters
         */
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);

        /**
         * @brief Allocates a buffer of the passed size, for the passed usage and with the correct properties to the vkBuffer reference
         */
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory);

        /**
         * @brief Copies the first buffer to the second shifted by a known byte sized offset
         */
        void transferMemoryToGPU(VkDeviceSize size, VkBuffer src, VkBuffer dst, VkDeviceSize src_offset, VkDeviceSize dst_offset);

        int vertices_size = 0;
        int indices_size = 0;

        // Instance number of the same objects that we want to draw
        uint32_t number_of_instances = 1;

        std::shared_ptr<LogicalDevice> l_device;
        std::unique_ptr<CommandBuffer> command_buffer;
        // Collection of descriptors (uniforms, textures etc..)
        std::unique_ptr<DescriptorSet> descriptor_set;

        // Attributes list for single vertex
        std::unique_ptr<VertexAttributes> attributes;

        // List of drawable elements
        std::vector<std::shared_ptr<DrawableElement>> elements;

        // Vector of vertices
        std::vector<float> vertices;

        // Vector of indices
        std::vector<uint32_t> indices;

        // Shaders for the pipeline
        const std::vector<std::shared_ptr<Shader>> shaders;

        // Allocated state. Represents if the buffers have already been allocated
        bool allocated = false;

        // Vulkan objects
        VkFence copy_fence = VK_NULL_HANDLE;

        VkBuffer vertex_staging_buffer = VK_NULL_HANDLE;
        VkDeviceMemory vertex_staging_buffer_memory = VK_NULL_HANDLE;
        VkBuffer index_staging_buffer = VK_NULL_HANDLE;
        VkDeviceMemory index_staging_buffer_memory = VK_NULL_HANDLE;
        VkBuffer vertex_buffer = VK_NULL_HANDLE;
        VkDeviceMemory vertex_buffer_memory = VK_NULL_HANDLE;
        VkBuffer index_buffer = VK_NULL_HANDLE;
        VkDeviceMemory index_buffer_memory = VK_NULL_HANDLE;
    };
}