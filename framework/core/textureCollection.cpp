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
    TextureCollection::TextureCollection(const std::shared_ptr<LogicalDevice> &lDevice, const VkCommandPool &pool, const std::vector<std::string> &filenames, uint32_t bindingIndex)
        : DescriptorElement(bindingIndex)
    {
        if (lDevice == nullptr)
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

        this->lDevice = lDevice;
        commandBuffer = std::make_unique<CommandBuffer>(lDevice, pool);

        // Reserve the vector space for the requested number of textures
        for (size_t i = 0; i < filenames.size(); i++)
        {
            textures.push_back(TextureDescriptor{});
            imageInfos.push_back(VkDescriptorImageInfo{});
        }

        // Create and store the textures on the GPU
        for (size_t i = 0; i < filenames.size(); i++)
        {
            const std::string &name = filenames[i];
            TextureDescriptor &descriptor = textures[i];
            VkDescriptorImageInfo &imageInfo = imageInfos[i];

            // Open the image
            stbi_uc *pixels = stbi_load(name.c_str(), &width, &height, &channels, STBI_rgb_alpha);

            // 4 = RGB + Alpha
            VkDeviceSize imageSize = width * height * 4;

            if (!pixels)
            {
                throw std::runtime_error("[Texture] Error opening texture image");
            }

            // Buffer to transfer the image to the GPU
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;

            // Image structs
            VkImageView textureImageView;
            VkSampler textureSampler;

            // Create the staging buffer and copy the image inside
            createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         stagingBuffer, stagingBufferMemory);

            // Map the buffer and transfer the image
            void *data;
            vkMapMemory(lDevice->getDevice(), stagingBufferMemory, 0, imageSize, 0, &data);

            // Transfer the memory
            memcpy(data, pixels, static_cast<size_t>(imageSize));

            // Unmap the memory
            vkUnmapMemory(lDevice->getDevice(), stagingBufferMemory);

            // Create the actual image in memory
            createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        descriptor.textureImage, descriptor.textureImageMemory);

            // Transition the image into the optimal transfer layout
            transitionImageLayout(descriptor.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            // Copy the buffer into the image
            copyBufferToImage(stagingBuffer, descriptor.textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

            // Remove the intermediate created buffer
            vkDestroyBuffer(lDevice->getDevice(), stagingBuffer, nullptr);
            vkFreeMemory(lDevice->getDevice(), stagingBufferMemory, nullptr);

            // Transfer the layout to be read only optimal
            transitionImageLayout(descriptor.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // Create the image view
            createImageView(descriptor.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, textureImageView);

            // Create the texture sampler TODO make this more configurable
            createSampler(textureSampler);

            // Set the image info
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            // Clean up the pixel array at the end
            stbi_image_free(pixels);
        }
    }

    TextureCollection::~TextureCollection()
    {
        for (size_t i = 0; i < textures.size(); i++)
        {
            TextureDescriptor& texture = textures[i];
            VkDescriptorImageInfo& imageInfo = imageInfos[i];

            if (texture.textureImage != VK_NULL_HANDLE)
            {
                vkDestroyImage(lDevice->getDevice(), texture.textureImage, nullptr);
            }
    
            if (texture.textureImageMemory != VK_NULL_HANDLE)
            {
                vkFreeMemory(lDevice->getDevice(), texture.textureImageMemory, nullptr);
            }
    
            if (imageInfo.imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(lDevice->getDevice(), imageInfo.imageView, nullptr);
            }
    
            if (imageInfo.sampler != VK_NULL_HANDLE)
            {
                vkDestroySampler(lDevice->getDevice(), imageInfo.sampler, nullptr);
            }
        }
    }

    const VkDescriptorSetLayoutBinding TextureCollection::getDescriptorSetLayoutBinding()
    {
        VkDescriptorSetLayoutBinding layoutBinding{};

        layoutBinding.binding = bindingIndex;
        layoutBinding.descriptorCount = 100;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.pImmutableSamplers = nullptr;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        return layoutBinding;
    }

    const VkDescriptorPoolSize TextureCollection::getPoolSize()
    {
        VkDescriptorPoolSize poolSize{};

        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 100;

        return poolSize;
    }

    const VkWriteDescriptorSet TextureCollection::getWriteDescriptorSet()
    {
        VkWriteDescriptorSet writeDescriptor{};

        writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptor.dstBinding = bindingIndex;
        writeDescriptor.dstArrayElement = 0;
        writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptor.descriptorCount = 100;
        writeDescriptor.pImageInfo = imageInfos.data();

        return writeDescriptor;
    }

    void TextureCollection::createSampler(VkSampler &sampler)
    {
        // Get the physical device properties to check the maximum anisotropic filtering
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(lDevice->getPhysicalDevice()->getDevice(), &properties);

        VkSamplerCreateInfo samplerInfo{};

        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        // TODO make them configurable (VK_FILTER_NEAREST or LINEAR)
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        // TODO make them configurable (REPEAT, MIRRORED, CLAMP..)
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        // TODO make it configurable
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        // TODO make it configurable. With VK_TRUE the coordinates are from 0 to width and from 0 to height
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        // TODO make it configurable
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(lDevice->getDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
        {
            throw std::runtime_error("[Texture] Impossible to crete texture sampler");
        }
    }

    void TextureCollection::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView &view)
    {
        VkImageViewCreateInfo createInfo{};

        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;

        // No mipmap level nor multiple layers
        createInfo.subresourceRange.aspectMask = aspectFlags;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(lDevice->getDevice(), &createInfo, nullptr, &view) != VK_SUCCESS)
        {
            throw std::runtime_error("[Texture] Error creating the image views");
        }
    }

    void TextureCollection::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        // Reset the command buffer for new sequence of commands
        vkResetCommandBuffer(commandBuffer->getCommandBuffer(), 0);

        // Start recording the command buffer
        commandBuffer->beginRecording();

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
        vkCmdCopyBufferToImage(commandBuffer->getCommandBuffer(),
                               buffer,
                               image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &region);

        // End the command buffer
        commandBuffer->stopRecording();

        // Submit the queue and wait for idle operation
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer->getCommandBuffer();

        vkQueueSubmit(lDevice->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(lDevice->getGraphicsQueue());
    }

    void TextureCollection::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        // Reset the command buffer for new sequence of commands
        vkResetCommandBuffer(commandBuffer->getCommandBuffer(), 0);

        // Start recording the command buffer
        commandBuffer->beginRecording();

        VkImageMemoryBarrier barrier{};

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        // Define the interesting transfers
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            throw std::invalid_argument("[Texture] Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer->getCommandBuffer(),
                             sourceStage, destinationStage,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        // End the command buffer
        commandBuffer->stopRecording();

        // Submit the queue and wait for idle operation
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer->getCommandBuffer();

        vkQueueSubmit(lDevice->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(lDevice->getGraphicsQueue());
    }

    void TextureCollection::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(lDevice->getDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
        {
            throw std::runtime_error("[Texture] Failed to create image");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(lDevice->getDevice(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(lDevice->getDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("[Texture] Failed to allocate image memory");
        }

        vkBindImageMemory(lDevice->getDevice(), image, imageMemory, 0);
    }

    void TextureCollection::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
    {
        // Create vertex buffer
        VkBufferCreateInfo bufferInfo{};

        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(lDevice->getDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("[Texture] Impossible to create the buffer");
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
            throw std::runtime_error("[Texture] Impossible to allocate the required memory on the GPU");
        }

        // Associate the buffer to the memory
        vkBindBufferMemory(lDevice->getDevice(), buffer, bufferMemory, 0);
    }

    uint32_t TextureCollection::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

        throw std::runtime_error("[Texture] Unable to find a suitable memory type");
    }
}