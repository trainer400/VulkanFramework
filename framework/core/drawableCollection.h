#pragma once

#include <vulkan/vulkan.h>

#include <core/drawableElement.h>
#include <core/commandPool.h>
#include <core/commandBuffer.h>
#include <core/shader.h>
#include <core/descriptorSet.h>
#include <core/commandBuffer.h>
#include <devices/logicalDevice.h>
#include <devices/physicalDevice.h>

#include <vector>
#include <memory>

namespace framework
{
    enum DrawableAttribute : uint32_t
    {
        F1 = VK_FORMAT_R32_SFLOAT,
        F2 = VK_FORMAT_R32G32_SFLOAT,
        F3 = VK_FORMAT_R32G32B32_SFLOAT,
        F4 = VK_FORMAT_R32G32B32A32_SFLOAT
    };

    class DrawableCollection
    {
    public:
        DrawableCollection(const std::shared_ptr<LogicalDevice> &lDevice, std::unique_ptr<DescriptorSet> descriptor, const VkCommandPool &pool, const std::vector<std::shared_ptr<Shader>> &shaders);
        ~DrawableCollection();

        /**
         * @brief Adds the drawable element inside the list unless the buffer has already been allocated
         * @throws Runtime Exception if the buffer has already been allocated
         */
        void addElement(const std::shared_ptr<DrawableElement> &element);

        /**
         * @brief Adds an attribute to the vertex description unless the buffer has already been allocated
         * @throws Runtime Exception if the buffer has already been allocated
         * @note Attributes are expressed as VkFormats in vulkan
         * float: VK_FORMAT_R32_SFLOAT
         * vec2: VK_FORMAT_R32G32_SFLOAT
         * vec3: VK_FORMAT_R32G32B32_SFLOAT
         * vec4: VK_FORMAT_R32G32B32A32_SFLOAT
         */
        void addAttribute(DrawableAttribute attribute);

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
        const VkBuffer &getVertexBuffer() { return vertexBuffer; }
        const VkBuffer &getIndexBuffer() { return indexBuffer; }
        uint32_t getVerticesNumber() { return vertices.size() / getAttributesSum(); }
        uint32_t getIndexSize() { return indices.size(); }
        uint32_t getNumberOfInstances() { return numberInstances; }
        bool isAllocated() { return allocated; }
        const std::vector<std::shared_ptr<Shader>> &getShaders() { return shaders; }
        inline const VkDescriptorPool &getDescriptorPool() { return descriptorSet->getDescriptorPool(); }
        inline const VkDescriptorSet &getDescriptorSet() { return descriptorSet->getDescriptorSet(); }
        inline const VkDescriptorSetLayout &getDescriptorSetLayout() { return descriptorSet->getDescriptorSetLayout(); }
        inline bool hasDescriptorSet() { return descriptorSet != nullptr; }

        // Setters
        void setNumberOfInstances(uint32_t instances) { numberInstances = instances; }

    private:
        /**
         * @brief Sums the number of floats per vertex
         */
        int getAttributesSum();

        /**
         * @brief Looks for the memory on the GPU that suits the passed parameters
         */
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);

        /**
         * @brief Allocates a buffer of the passed size, for the passed usage and with the correct properties to the vkBuffer reference
         */
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);

        /**
         * @brief Copies the first buffer to the second shifted by a known byte sized offset
         */
        void transferMemoryToGPU(VkDeviceSize size, VkBuffer src, VkBuffer dst, VkDeviceSize srcOffset, VkDeviceSize dstOffset);

        int vertexSize = 0;
        int indicesSize = 0;

        // Instance number of the same objects that we want to draw
        uint32_t numberInstances = 1;

        std::shared_ptr<LogicalDevice> lDevice;
        std::unique_ptr<CommandBuffer> commandBuffer;
        // Collection of descriptors (uniforms, textures etc..)
        std::unique_ptr<DescriptorSet> descriptorSet;

        // Attributes list for single vertex
        std::vector<DrawableAttribute> attributes;

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
        VkFence copyFence = VK_NULL_HANDLE;

        VkBuffer vertexStagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexStagingBufferMemory = VK_NULL_HANDLE;
        VkBuffer indexStagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexStagingBufferMemory = VK_NULL_HANDLE;
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    };
}