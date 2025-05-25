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
        RenderPass(const std::shared_ptr<LogicalDevice> &l_device, const VkExtent2D &extent, const VkSurfaceFormatKHR &format, DepthTestType depth = DepthTestType::NONE);
        ~RenderPass();

        /**
         * @brief Recreates the render pass cleaning up the previous one. Usually due to a window resize
         */
        void recreateRenderPass(const VkExtent2D &extent, const VkSurfaceFormatKHR &format);

        // Render pass functions
        void begin(const VkCommandBuffer &cmd_buffer, const VkFramebuffer &frame_buffer, const VkExtent2D &extent, const VkClearValue &clear_color);
        void end(const VkCommandBuffer &cmd_buffer);

        // Getters
        const VkRenderPass &getRenderPass() { return render_pass; }
        const VkImageView &getDepthImageView() { return depth_image_view; }
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
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory);

        /**
         * @brief Looks for the memory on the GPU that suits the passed parameters
         */
        uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags flags);

        /**
         * @brief Creates the corresponding image view for the created depth image
         */
        void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView &view);

        std::shared_ptr<LogicalDevice> l_device;
        DepthTestType depth;

        VkRenderPass render_pass = VK_NULL_HANDLE;

        // Depth buffer
        VkImage depth_image = VK_NULL_HANDLE;
        VkDeviceMemory depth_image_memory = VK_NULL_HANDLE;
        VkImageView depth_image_view = VK_NULL_HANDLE;
    };
}