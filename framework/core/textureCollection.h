#pragma once

#include <devices/logicalDevice.h>
#include <core/commandPool.h>
#include <core/commandBuffer.h>
#include <core/descriptorElement.h>

#include <memory>
#include <vector>

namespace framework
{
    class TextureCollection : public DescriptorElement
    {
    public:
        TextureCollection(const std::shared_ptr<LogicalDevice> &lDevice, const VkCommandPool &pool, const std::vector<std::string> &filenames, uint32_t bindingIndex);
        ~TextureCollection();

        // Getters
        const VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding() override;
        const VkDescriptorPoolSize getPoolSize() override;
        const VkWriteDescriptorSet getWriteDescriptorSet() override;

    private:
        /**
         * @brief Creates the texture image sampler for shader
         */
        void createSampler(VkSampler &sampler);

        /**
         * @brief Creates the corresponding image view for the created depth image
         */
        void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView &view);

        /**
         * @brief Copies a buffer content into the specified image
         */
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

        /**
         * @brief Transforms the layout of an image into a new one
         */
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

        /**
         * @brief Allocates the image inside the memory
         */
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);

        /**
         * @brief Allocates a buffer of the passed size, for the passed usage and with the correct properties to the vkBuffer reference
         */
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);

        /**
         * @brief Looks for the memory on the GPU that suits the passed parameters
         */
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);

        struct TextureDescriptor
        {
            // Image view
            VkImage textureImage;
            VkDeviceMemory textureImageMemory;
        };

        std::vector<TextureDescriptor> textures;
        // Image descriptors
        std::vector<VkDescriptorImageInfo> imageInfos;


        // Framework objects
        std::shared_ptr<LogicalDevice> lDevice;
        std::unique_ptr<CommandBuffer> commandBuffer;

        // Image info
        int width, height, channels;
    };
}