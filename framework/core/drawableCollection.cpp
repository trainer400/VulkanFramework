#include "drawableCollection.h"

#include <stdexcept>
#include <cstring>

namespace framework
{
    DrawableCollection::DrawableCollection(const std::shared_ptr<LogicalDevice> &l_device, std::unique_ptr<DescriptorSet> descriptor, const VkCommandPool &pool, const std::vector<std::shared_ptr<Shader>> &shaders)
        : shaders(shaders)
    {
        if (l_device == nullptr)
        {
            throw std::runtime_error("[DrawableCollection] Null logical device");
        }

        if (pool == nullptr)
        {
            throw std::runtime_error("[DrawableCollection] Null command pool instance");
        }

        this->l_device = l_device;
        this->descriptor_set = std::move(descriptor);

        // Create the command buffer
        command_buffer = std::make_unique<CommandBuffer>(l_device, pool);

        // Create the fence for memory transfer
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        if (vkCreateFence(l_device->getDevice(), &fence_info, nullptr, &copy_fence) != VK_SUCCESS)
        {
            throw std::runtime_error("[DrawableCollection] Error creating copy fence");
        }
    }

    DrawableCollection::~DrawableCollection()
    {
        if (vertex_staging_buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(l_device->getDevice(), vertex_staging_buffer, nullptr);
        }

        if (vertex_staging_buffer_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(l_device->getDevice(), vertex_staging_buffer_memory, nullptr);
        }

        if (index_staging_buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(l_device->getDevice(), index_staging_buffer, nullptr);
        }

        if (index_staging_buffer_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(l_device->getDevice(), index_staging_buffer_memory, nullptr);
        }

        if (vertex_buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(l_device->getDevice(), vertex_buffer, nullptr);
        }

        if (vertex_buffer_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(l_device->getDevice(), vertex_buffer_memory, nullptr);
        }

        if (index_buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(l_device->getDevice(), index_buffer, nullptr);
        }

        if (index_buffer_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(l_device->getDevice(), index_buffer_memory, nullptr);
        }

        if (copy_fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(l_device->getDevice(), copy_fence, nullptr);
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
            // In case of empty vertex attributes, use the ones with the new element
            if (elements.size() == 0)
            {
                attributes = std::make_unique<VertexAttributes>(element->getVertexAttributes());
            }

            // Check that the new element has the same vertex attributes of the other ones in the collection
            if (!(*attributes.get() == element->getVertexAttributes()))
            {
                throw std::runtime_error("[DrawableCollection] New element vertex attributes differ from exsiting elements inside the collection");
            }

            elements.push_back(element);

            // Increase the counters
            vertices_size += element->getVertices().size();
            indices_size += element->getIndices().size();
        }
        else
        {
            throw std::runtime_error("[DrawableCollection] The buffer has already been allocated");
        }
    }

    void DrawableCollection::allocate()
    {
        if (allocated)
        {
            throw std::runtime_error("[DrawableCollection] The buffer has already been allocated");
        }

        if (elements.size() == 0)
        {
            throw std::runtime_error("[DrawableCollection] Allocate function called but empty element list");
        }

        // Set the allocated flag
        allocated = true;

        // Get the size in bytes of the struct
        int size_of_struct = getAttributesSum();
        int vertex_index = 0;

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
                indices.push_back(index[j] + vertex_index / size_of_struct);
            }

            // Update the indices
            vertex_index += vertex.size();
        }

        VkDeviceSize vertex_buffer_size = sizeof(vertices[0]) * vertices.size();
        VkDeviceSize index_buffer_size = sizeof(indices[0]) * indices.size();

        createBuffer(vertex_buffer_size,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     vertex_staging_buffer, vertex_staging_buffer_memory);

        createBuffer(index_buffer_size,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     index_staging_buffer, index_staging_buffer_memory);

        createBuffer(vertex_buffer_size,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     vertex_buffer, vertex_buffer_memory);

        createBuffer(index_buffer_size,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     index_buffer, index_buffer_memory);

        // Fill the vertex buffer
        void *data;

        // Map the GPU memory into the RAM
        vkMapMemory(l_device->getDevice(), vertex_staging_buffer_memory, 0, vertex_buffer_size, 0, &data);

        // Copy the data to the shared memory
        memcpy(data, vertices.data(), (size_t)vertex_buffer_size);

        // Unmap the shared memory
        vkUnmapMemory(l_device->getDevice(), vertex_staging_buffer_memory);

        // Fill the index buffer
        vkMapMemory(l_device->getDevice(), index_staging_buffer_memory, 0, index_buffer_size, 0, &data);

        // Copy the data to the shared memory
        memcpy(data, indices.data(), (size_t)index_buffer_size);

        // Unmap the shared memory
        vkUnmapMemory(l_device->getDevice(), index_staging_buffer_memory);

        // Transfer the data from staging area to the GPU memory
        transferMemoryToGPU(vertex_buffer_size, vertex_staging_buffer, vertex_buffer, 0, 0);
        transferMemoryToGPU(index_buffer_size, index_staging_buffer, index_buffer, 0, 0);
    }

    void DrawableCollection::updateElements()
    {
        int vertex_index = 0;
        int element_index = 0;
        int size_of_attributes = getAttributesSum();

        int vSize = 0, eSize = 0;

        for (int i = 0; i < elements.size(); i++)
        {
            // Keep track of global vertices and indices sizes
            vSize += elements[i]->getVertices().size();
            eSize += elements[i]->getIndices().size();

            if (elements[i]->isUpdated())
            {
                // Save the reference of the two vectors
                const std::vector<float> &v = elements[i]->getVertices();

                // Copy the new changed vertices inside the vertex vector
                for (int j = 0; j < v.size(); j++)
                {
                    vertices[j + vertex_index] = v[j];
                }

                // Compute the byte offset
                VkDeviceSize verticesOffset = vertex_index * sizeof(float);
                VkDeviceSize verticesSize = v.size() * sizeof(float);

                // Map the changes inside the staging buffer
                void *data;

                // Map the GPU memory into the RAM
                vkMapMemory(l_device->getDevice(), vertex_staging_buffer_memory, verticesOffset, verticesSize, 0, &data);

                // Copy the data to the GPU memory
                memcpy(data, &vertices.data()[vertex_index], (size_t)verticesSize);

                // Unmap the GPU memory
                vkUnmapMemory(l_device->getDevice(), vertex_staging_buffer_memory);

                // Transfer the change into the GPU memory
                transferMemoryToGPU(verticesSize, vertex_staging_buffer, vertex_buffer, verticesOffset, verticesOffset);

                // Flag the element as updated
                elements[i]->setUpdated();
            }

            vertex_index += elements[i]->getVertices().size();
            element_index += elements[i]->getIndices().size();
        }

        // The array dimensions is changed
        if (vSize != vertices_size || eSize != indices_size)
        {
            throw std::runtime_error("[DrawableCollection] Changed vertices of elements size");
        }
    }

    VkVertexInputBindingDescription DrawableCollection::getBindingDescription()
    {
        VkVertexInputBindingDescription result{};

        // Collect the size of the overall struct
        int size_of_struct = getAttributesSum();

        // Data is populated only if the buffer has been allocated
        if (allocated)
        {
            result.binding = 0;
            result.stride = size_of_struct * sizeof(float);
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
            for (int i = 0; i < attributes->getVertexAttributes().size(); i++)
            {
                VkVertexInputAttributeDescription description{};

                description.binding = 0;
                description.location = i;
                description.format = static_cast<VkFormat>(attributes->getVertexAttributes()[i]);
                description.offset = offset;

                // Update the offset
                switch (attributes->getVertexAttributes()[i])
                {
                case VertexAttributes::DrawableAttribute::F1:
                case VertexAttributes::DrawableAttribute::I1:
                    offset += 1 * sizeof(float);
                    break;
                case VertexAttributes::DrawableAttribute::F2:
                    offset += 2 * sizeof(float);
                    break;
                case VertexAttributes::DrawableAttribute::F3:
                    offset += 3 * sizeof(float);
                    break;
                case VertexAttributes::DrawableAttribute::F4:
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
        int size_of_struct = 0;
        for (int i = 0; i < attributes->getVertexAttributes().size(); i++)
        {
            switch (attributes->getVertexAttributes()[i])
            {
            case VertexAttributes::DrawableAttribute::F1:
            case VertexAttributes::DrawableAttribute::I1:
                size_of_struct += 1;
                break;
            case VertexAttributes::DrawableAttribute::F2:
                size_of_struct += 2;
                break;
            case VertexAttributes::DrawableAttribute::F3:
                size_of_struct += 3;
                break;
            case VertexAttributes::DrawableAttribute::F4:
                size_of_struct += 4;
                break;
            }
        }

        return size_of_struct;
    }

    uint32_t DrawableCollection::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;

        // Enumerate the memory properties
        vkGetPhysicalDeviceMemoryProperties(l_device->getPhysicalDevice()->getDevice(), &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("[DrawableCollection] Unable to find a suitable memory type");
    }

    void DrawableCollection::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory)
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

    void DrawableCollection::transferMemoryToGPU(VkDeviceSize size, VkBuffer src, VkBuffer dst, VkDeviceSize src_offset, VkDeviceSize dst_offset)
    {
        // Reset the fence
        vkResetFences(l_device->getDevice(), 1, &copy_fence);

        // Reset the command buffer
        vkResetCommandBuffer(command_buffer->getCommandBuffer(), 0);

        // Record the command buffer to transfer the memory
        command_buffer->beginRecording();

        VkBufferCopy copy_region{};
        copy_region.srcOffset = src_offset;
        copy_region.dstOffset = dst_offset;
        copy_region.size = size;

        vkCmdCopyBuffer(command_buffer->getCommandBuffer(), src, dst, 1, &copy_region);

        // End the command buffer recording
        command_buffer->stopRecording();

        // Submit the command buffer to the graphics queue (supports data movement)
        VkSubmitInfo submit_info{};

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer->getCommandBuffer();

        vkQueueSubmit(l_device->getGraphicsQueue(), 1, &submit_info, copy_fence);

        // Wait for copy to be completed
        vkWaitForFences(l_device->getDevice(), 1, &copy_fence, VK_TRUE, UINT64_MAX);
    }
}