#include "textureCollection.h"

#include <stdexcept>
#include <cstring>
#include <iostream>

#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STB_IMAGE_IMPLEMENTATION
#include <libs/stb_image.h>
#endif

namespace framework
{
    TextureCollection::TextureCollection(const std::shared_ptr<LogicalDevice> &l_device, const VkCommandPool &pool, const std::vector<std::string> &filenames, uint32_t binding_index)
        : DescriptorElement(binding_index)
    {
        if (l_device == nullptr)
        {
            throw std::runtime_error("[Texture] Null logical device instance");
        }

        if (filenames.size() == 0)
        {
            throw std::runtime_error("[Texture] Null filenames");
        }

        if (pool == nullptr)
        {
            throw std::runtime_error("[Texture] Null command pool");
        }

        this->l_device = l_device;
        command_buffer = std::make_unique<CommandBuffer>(l_device, pool);

        // Reserve the vector space for the requested number of textures
        for (size_t i = 0; i < filenames.size(); i++)
        {
            textures.push_back(TextureDescriptor{});
            image_infos.push_back(VkDescriptorImageInfo{});
        }

        // Create and store the textures on the GPU
        for (size_t i = 0; i < filenames.size(); i++)
        {
            const std::string &name = filenames[i];
            TextureDescriptor &descriptor = textures[i];
            VkDescriptorImageInfo &image_info = image_infos[i];

            // Open the image
            stbi_uc *pixels = stbi_load(name.c_str(), &width, &height, &channels, STBI_rgb_alpha);

            // 4 = RGB + Alpha
            VkDeviceSize image_size = width * height * 4;

            if (!pixels)
            {
                throw std::runtime_error("[Texture] Error opening texture image");
            }

            // Buffer to transfer the image to the GPU
            VkBuffer staging_buffer;
            VkDeviceMemory staging_buffer_memory;

            // Image structs
            VkImageView texture_image_view;
            VkSampler texture_sampler;

            // Create the staging buffer and copy the image inside
            createBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         staging_buffer, staging_buffer_memory);

            // Map the buffer and transfer the image
            void *data;
            vkMapMemory(l_device->getDevice(), staging_buffer_memory, 0, image_size, 0, &data);

            // Transfer the memory
            memcpy(data, pixels, static_cast<size_t>(image_size));

            // Unmap the memory
            vkUnmapMemory(l_device->getDevice(), staging_buffer_memory);

            // Create the actual image in memory
            createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        descriptor.texture_image, descriptor.texture_image_memory);

            // Transition the image into the optimal transfer layout
            transitionImageLayout(descriptor.texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            // Copy the buffer into the image
            copyBufferToImage(staging_buffer, descriptor.texture_image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

            // Remove the intermediate created buffer
            vkDestroyBuffer(l_device->getDevice(), staging_buffer, nullptr);
            vkFreeMemory(l_device->getDevice(), staging_buffer_memory, nullptr);

            // Transfer the layout to be read only optimal
            transitionImageLayout(descriptor.texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // Create the image view
            createImageView(descriptor.texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, texture_image_view);

            // Create the texture sampler TODO make this more configurable
            createSampler(texture_sampler);

            // Set the image info
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = texture_image_view;
            image_info.sampler = texture_sampler;

            // Clean up the pixel array at the end
            stbi_image_free(pixels);
        }
    }

    TextureCollection::~TextureCollection()
    {
        for (size_t i = 0; i < textures.size(); i++)
        {
            TextureDescriptor &texture = textures[i];
            VkDescriptorImageInfo &image_info = image_infos[i];

            if (texture.texture_image != VK_NULL_HANDLE)
            {
                vkDestroyImage(l_device->getDevice(), texture.texture_image, nullptr);
            }

            if (texture.texture_image_memory != VK_NULL_HANDLE)
            {
                vkFreeMemory(l_device->getDevice(), texture.texture_image_memory, nullptr);
            }

            if (image_info.imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(l_device->getDevice(), image_info.imageView, nullptr);
            }

            if (image_info.sampler != VK_NULL_HANDLE)
            {
                vkDestroySampler(l_device->getDevice(), image_info.sampler, nullptr);
            }
        }
    }

    const VkDescriptorSetLayoutBinding TextureCollection::getDescriptorSetLayoutBinding()
    {
        VkDescriptorSetLayoutBinding layout_binding{};

        layout_binding.binding = binding_index;
        layout_binding.descriptorCount = textures.size();
        layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layout_binding.pImmutableSamplers = nullptr;
        layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        return layout_binding;
    }

    const VkDescriptorPoolSize TextureCollection::getPoolSize()
    {
        VkDescriptorPoolSize pool_size{};

        pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_size.descriptorCount = textures.size();

        return pool_size;
    }

    const VkWriteDescriptorSet TextureCollection::getWriteDescriptorSet()
    {
        VkWriteDescriptorSet write_descriptor{};

        write_descriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor.dstBinding = binding_index;
        write_descriptor.dstArrayElement = 0;
        write_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptor.descriptorCount = textures.size();
        write_descriptor.pImageInfo = image_infos.data();

        return write_descriptor;
    }

    void TextureCollection::createSampler(VkSampler &sampler)
    {
        // Get the physical device properties to check the maximum anisotropic filtering
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(l_device->getPhysicalDevice()->getDevice(), &properties);

        VkSamplerCreateInfo sampler_info{};

        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        // TODO make them configurable (VK_FILTER_NEAREST or LINEAR)
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;

        // TODO make them configurable (REPEAT, MIRRORED, CLAMP..)
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        // TODO make it configurable
        sampler_info.anisotropyEnable = VK_TRUE;
        sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        // TODO make it configurable. With VK_TRUE the coordinates are from 0 to width and from 0 to height
        sampler_info.unnormalizedCoordinates = VK_FALSE;

        // TODO make it configurable
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;

        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = 0.0f;

        if (vkCreateSampler(l_device->getDevice(), &sampler_info, nullptr, &sampler) != VK_SUCCESS)
        {
            throw std::runtime_error("[Texture] Impossible to crete texture sampler");
        }
    }

    void TextureCollection::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView &view)
    {
        VkImageViewCreateInfo create_info{};

        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = image;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = format;

        // No mipmap level nor multiple layers
        create_info.subresourceRange.aspectMask = aspect_flags;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(l_device->getDevice(), &create_info, nullptr, &view) != VK_SUCCESS)
        {
            throw std::runtime_error("[Texture] Error creating the image views");
        }
    }

    void TextureCollection::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        // Reset the command buffer for new sequence of commands
        vkResetCommandBuffer(command_buffer->getCommandBuffer(), 0);

        // Start recording the command buffer
        command_buffer->beginRecording();

        // Copy the buffer
        VkBufferImageCopy region{};

        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        // Assumes that the image has already been transitioned to the optimal layout
        vkCmdCopyBufferToImage(command_buffer->getCommandBuffer(),
                               buffer,
                               image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &region);

        // End the command buffer
        command_buffer->stopRecording();

        // Submit the queue and wait for idle operation
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer->getCommandBuffer();

        vkQueueSubmit(l_device->getGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(l_device->getGraphicsQueue());
    }

    void TextureCollection::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
    {
        // Reset the command buffer for new sequence of commands
        vkResetCommandBuffer(command_buffer->getCommandBuffer(), 0);

        // Start recording the command buffer
        command_buffer->beginRecording();

        VkImageMemoryBarrier barrier{};

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags destination_stage;

        // Define the interesting transfers
        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            throw std::invalid_argument("[Texture] Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(command_buffer->getCommandBuffer(),
                             source_stage, destination_stage,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        // End the command buffer
        command_buffer->stopRecording();

        // Submit the queue and wait for idle operation
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer->getCommandBuffer();

        vkQueueSubmit(l_device->getGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(l_device->getGraphicsQueue());
    }

    void TextureCollection::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory)
    {
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = width;
        image_info.extent.height = height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.format = format;
        image_info.tiling = tiling;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = usage;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(l_device->getDevice(), &image_info, nullptr, &image) != VK_SUCCESS)
        {
            throw std::runtime_error("[Texture] Failed to create image");
        }

        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(l_device->getDevice(), image, &memory_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = memory_requirements.size;
        alloc_info.memoryTypeIndex = findMemoryType(memory_requirements.memoryTypeBits, properties);

        if (vkAllocateMemory(l_device->getDevice(), &alloc_info, nullptr, &image_memory) != VK_SUCCESS)
        {
            throw std::runtime_error("[Texture] Failed to allocate image memory");
        }

        vkBindImageMemory(l_device->getDevice(), image, image_memory, 0);
    }

    void TextureCollection::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory)
    {
        // Create vertex buffer
        VkBufferCreateInfo buffer_info{};

        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(l_device->getDevice(), &buffer_info, nullptr, &buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("[Texture] Impossible to create the buffer");
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
            throw std::runtime_error("[Texture] Impossible to allocate the required memory on the GPU");
        }

        // Associate the buffer to the memory
        vkBindBufferMemory(l_device->getDevice(), buffer, buffer_memory, 0);
    }

    uint32_t TextureCollection::findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
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

        throw std::runtime_error("[Texture] Unable to find a suitable memory type");
    }
}