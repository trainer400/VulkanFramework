#include "drawableCollection.h"

#include <stdexcept>
#include <cstring>

namespace framework
{
    DrawableCollection::DrawableCollection(const std::shared_ptr<LogicalDevice> &lDevice, std::unique_ptr<DescriptorSet> descriptor, const VkCommandPool &pool, const std::vector<std::shared_ptr<Shader>> &shaders)
        : shaders(shaders)
    {
        if (lDevice == nullptr)
        {
            throw std::runtime_error("[DrawableCollection] Null logical device");
        }

        if (pool == nullptr)
        {
            throw std::runtime_error("[DrawableCollection] Null command pool instance");
        }

        this->lDevice = lDevice;
        this->descriptorSet = std::move(descriptor);

        // Create the command buffer
        commandBuffer = std::make_unique<CommandBuffer>(lDevice, pool);

        // Create the fence for memory transfer
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        if (vkCreateFence(lDevice->getDevice(), &fenceInfo, nullptr, &copyFence) != VK_SUCCESS)
        {
            throw std::runtime_error("[DrawableCollection] Error creating copy fence");
        }
    }

    DrawableCollection::~DrawableCollection()
    {
        if (vertexStagingBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(lDevice->getDevice(), vertexStagingBuffer, nullptr);
        }

        if (vertexStagingBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(lDevice->getDevice(), vertexStagingBufferMemory, nullptr);
        }

        if (indexStagingBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(lDevice->getDevice(), indexStagingBuffer, nullptr);
        }

        if (indexStagingBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(lDevice->getDevice(), indexStagingBufferMemory, nullptr);
        }

        if (vertexBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(lDevice->getDevice(), vertexBuffer, nullptr);
        }

        if (vertexBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(lDevice->getDevice(), vertexBufferMemory, nullptr);
        }

        if (indexBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(lDevice->getDevice(), indexBuffer, nullptr);
        }

        if (indexBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(lDevice->getDevice(), indexBufferMemory, nullptr);
        }

        if (copyFence != VK_NULL_HANDLE)
        {
            vkDestroyFence(lDevice->getDevice(), copyFence, nullptr);
        }
    }

    void DrawableCollection::addElement(const std::shared_ptr<DrawableElement> &element)
    {
        if (element == nullptr)
        {
            throw std::runtime_error("[DrawableCollection] Null element");
        }

        if (!allocated)
        {
            elements.push_back(element);

            // Increase the counters
            vertexSize += element->getVertices().size();
            indicesSize += element->getIndices().size();
        }
        else
        {
            throw std::runtime_error("[DrawableCollection] The buffer has already been allocated");
        }
    }

    void DrawableCollection::addAttribute(DrawableAttribute attribute)
    {
        if (!allocated)
        {
            attributes.push_back(attribute);
        }
        else
        {
            throw std::runtime_error("[DrawableCollection] Buffer already allocated");
        }
    }

    void DrawableCollection::allocate()
    {
        if (allocated)
        {
            throw std::runtime_error("[DrawableCollection] The buffer has already been allocated");
        }

        // Set the allocated flag
        allocated = true;

        // Get the size in bytes of the struct
        int sizeOfStruct = getAttributesSum();
        int vertexIndex = 0;

        // Allocate the vectors befor creating the Vulkan buffer
        for (int i = 0; i < elements.size(); i++)
        {
            auto &vertex = elements[i]->getVertices();
            auto &index = elements[i]->getIndices();

            // Add the vertices on the bottom of the vector
            vertices.insert(vertices.end(), vertex.begin(), vertex.end());

            // Manipulate the indices before inserting them into the vector
            for (int j = 0; j < index.size(); j++)
            {
                indices.push_back(index[j] + vertexIndex / sizeOfStruct);
            }

            // Update the indices
            vertexIndex += vertex.size();
        }

        VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
        VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

        createBuffer(vertexBufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     vertexStagingBuffer, vertexStagingBufferMemory);

        createBuffer(indexBufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     indexStagingBuffer, indexStagingBufferMemory);

        createBuffer(vertexBufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     vertexBuffer, vertexBufferMemory);

        createBuffer(indexBufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     indexBuffer, indexBufferMemory);

        // Fill the vertex buffer
        void *data;

        // Map the GPU memory into the RAM
        vkMapMemory(lDevice->getDevice(), vertexStagingBufferMemory, 0, vertexBufferSize, 0, &data);

        // Copy the data to the shared memory
        memcpy(data, vertices.data(), (size_t)vertexBufferSize);

        // Unmap the shared memory
        vkUnmapMemory(lDevice->getDevice(), vertexStagingBufferMemory);

        // Fill the index buffer
        vkMapMemory(lDevice->getDevice(), indexStagingBufferMemory, 0, indexBufferSize, 0, &data);

        // Copy the data to the shared memory
        memcpy(data, indices.data(), (size_t)indexBufferSize);

        // Unmap the shared memory
        vkUnmapMemory(lDevice->getDevice(), indexStagingBufferMemory);

        // Transfer the data from staging area to the GPU memory
        transferMemoryToGPU(vertexBufferSize, vertexStagingBuffer, vertexBuffer, 0, 0);
        transferMemoryToGPU(indexBufferSize, indexStagingBuffer, indexBuffer, 0, 0);
    }

    void DrawableCollection::updateElements()
    {
        int vertexIndex = 0;
        int elementIndex = 0;
        int sizeOfAttributes = getAttributesSum();

        int vSize = 0, eSize = 0;

        for (int i = 0; i < elements.size(); i++)
        {
            // Keep track of global vertices and indices sizes
            vSize += elements[i]->getVertices().size();
            eSize += elements[i]->getIndices().size();

            if (elements[i]->isUpdated())
            {
                // Save the reference of the two vectors
                const std::vector<float> v = elements[i]->getVertices();

                // Copy the new changed vertices inside the vertex vector
                for (int j = 0; j < v.size(); j++)
                {
                    vertices[j + vertexIndex] = v[j];
                }

                // Compute the byte offset
                VkDeviceSize verticesOffset = vertexIndex * sizeof(float);
                VkDeviceSize verticesSize = v.size() * sizeof(float);

                // Map the changes inside the staging buffer
                void *data;

                // Map the GPU memory into the RAM
                vkMapMemory(lDevice->getDevice(), vertexStagingBufferMemory, verticesOffset, verticesSize, 0, &data);

                // Copy the data to the GPU memory
                memcpy(data, &vertices.data()[vertexIndex], (size_t)verticesSize);

                // Unmap the GPU memory
                vkUnmapMemory(lDevice->getDevice(), vertexStagingBufferMemory);

                // Transfer the change into the GPU memory
                transferMemoryToGPU(verticesSize, vertexStagingBuffer, vertexBuffer, verticesOffset, verticesOffset);

                // Flag the element as updated
                elements[i]->setUpdated();
            }

            vertexIndex += elements[i]->getVertices().size();
            elementIndex += elements[i]->getIndices().size();
        }

        // The array dimensions is changed
        if (vSize != vertexSize || eSize != indicesSize)
        {
            throw std::runtime_error("[DrawableCollection] Changed vertices of elements size");
        }
    }

    VkVertexInputBindingDescription DrawableCollection::getBindingDescription()
    {
        VkVertexInputBindingDescription result{};

        // Collect the size of the overall struct
        int sizeOfStruct = getAttributesSum();

        // Data is populated only if the buffer has been allocated
        if (allocated)
        {
            result.binding = 0;
            result.stride = sizeOfStruct * sizeof(float);
            // TODO make this configurable ?
            result.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        }

        return result;
    }

    std::vector<VkVertexInputAttributeDescription> DrawableCollection::getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> descriptions{};

        // Data is populated only if the buffer has been allocated
        if (allocated)
        {
            uint32_t offset = 0;
            for (int i = 0; i < attributes.size(); i++)
            {
                VkVertexInputAttributeDescription description{};

                description.binding = 0;
                description.location = i;
                description.format = static_cast<VkFormat>(attributes[i]);
                description.offset = offset;

                // Update the offset
                switch (attributes[i])
                {
                case F1:
                    offset += 1 * sizeof(float);
                    break;
                case F2:
                    offset += 2 * sizeof(float);
                    break;
                case F3:
                    offset += 3 * sizeof(float);
                    break;
                case F4:
                    offset += 4 * sizeof(float);
                    break;
                }

                // Add the description at the end
                descriptions.push_back(description);
            }
        }

        return descriptions;
    }

    int DrawableCollection::getAttributesSum()
    {
        int sizeOfStruct = 0;
        for (int i = 0; i < attributes.size(); i++)
        {
            switch (attributes[i])
            {
            case F1:
                sizeOfStruct += 1;
                break;
            case F2:
                sizeOfStruct += 2;
                break;
            case F3:
                sizeOfStruct += 3;
                break;
            case F4:
                sizeOfStruct += 4;
                break;
            }
        }

        return sizeOfStruct;
    }

    uint32_t DrawableCollection::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

    void DrawableCollection::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
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

    void DrawableCollection::transferMemoryToGPU(VkDeviceSize size, VkBuffer src, VkBuffer dst, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
    {
        // Reset the fence
        vkResetFences(lDevice->getDevice(), 1, &copyFence);

        // Reset the command buffer
        vkResetCommandBuffer(commandBuffer->getCommandBuffer(), 0);

        // Record the command buffer to transfer the memory
        commandBuffer->beginRecording();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;
        copyRegion.size = size;

        vkCmdCopyBuffer(commandBuffer->getCommandBuffer(), src, dst, 1, &copyRegion);

        // End the command buffer recording
        commandBuffer->stopRecording();

        // Submit the command buffer to the graphics queue (supports data movement)
        VkSubmitInfo submitInfo{};

        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer->getCommandBuffer();

        vkQueueSubmit(lDevice->getGraphicsQueue(), 1, &submitInfo, copyFence);

        // Wait for copy to be completed
        vkWaitForFences(lDevice->getDevice(), 1, &copyFence, VK_TRUE, UINT64_MAX);
    }
}