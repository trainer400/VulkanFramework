#pragma once

#include <devices/logicalDevice.h>
#include <core/swapChain.h>

#include <vulkan/vulkan.h>

namespace framework
{
    enum DepthTestType : uint32_t
    {
        NONE = 0,
        DEPTH_32 = VK_FORMAT_D32_SFLOAT,
        DEPTH_24_STENCIL_8 = VK_FORMAT_D32_SFLOAT_S8_UINT,
        DEPTH_16_STENCIL_8 = VK_FORMAT_D24_UNORM_S8_UINT
    };

    class RenderPass
    {
    public:
        RenderPass(const std::shared_ptr<LogicalDevice> &lDevice, const VkExtent2D &extent, const VkSurfaceFormatKHR &format, DepthTestType depth = DepthTestType::NONE);
        ~RenderPass();

        /**
         * @brief Recreates the render pass cleaning up the previous one. Usually due to a window resize
         */
        void recreateRenderPass(const VkExtent2D &extent, const VkSurfaceFormatKHR &format);

        // Getters
        const VkRenderPass &getRenderPass() { return renderPass; }
        const VkImageView &getDepthImageView() { return depthImageView; }
        const DepthTestType &getDepthTestType() { return depth; }

    private:
        /**
         * @brief Cleans every vulkan object of this class
         */
        void cleanup();

        /**
         * @brief Creates the vulkan instance of the render pass with also the image view in case of depth buffer
         */
        void createRenderPass(const VkExtent2D &extent, const VkSurfaceFormatKHR &format);

        /**
         * @brief Checks that the format is valid and throws an exception if not
         */
        void checkFormat(VkFormat candidate, VkImageTiling tiling, VkFormatFeatureFlags features);

        /**
         * @brief Allocates the image inside the memory
         */
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);

        /**
         * @brief Looks for the memory on the GPU that suits the passed parameters
         */
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);

        /**
         * @brief Creates the corresponding image view for the created depth image
         */
        void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView &view);

        std::shared_ptr<LogicalDevice> lDevice;
        DepthTestType depth;

        VkRenderPass renderPass = VK_NULL_HANDLE;

        // Depth buffer
        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;
    };
}