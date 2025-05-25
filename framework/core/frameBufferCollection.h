#pragma once

#include <core/swapChain.h>
#include <core/renderPass.h>
#include <devices/logicalDevice.h>

#include <vulkan/vulkan.h>
#include <vector>

namespace framework
{
    class FrameBufferCollection
    {
    public:
        FrameBufferCollection(const std::shared_ptr<LogicalDevice> &l_device,
                              const std::vector<VkImageView> &image_views, const VkExtent2D &extent,
                              const DepthTestType &depth_test_type, const VkImageView &depth_image_view, const VkRenderPass &render_pass);
        ~FrameBufferCollection();

        /**
         * @brief Recreates the frame buffer (usually called after window resize)
         */
        void recreateFrameBuffer(const std::vector<VkImageView> &image_views, const VkExtent2D &extent,
                                 const DepthTestType &depth_test_type, const VkImageView &depth_image_view, const VkRenderPass &render_pass);

        // Getter
        const std::vector<VkFramebuffer> &getFrameBuffers() { return frame_buffers; }

    private:
        /**
         * @brief Creates the actual frame buffer. It is called by the constructor and by the
         * frame buffer re-creation method.
         */
        void createFrameBuffer(const std::vector<VkImageView> &image_views, const VkExtent2D &extent,
                               const DepthTestType &depth_test_type, const VkImageView &depth_image_view, const VkRenderPass &render_pass);

        /**
         * @brief Cleans everything and removes the frame buffers
         */
        void cleanup();

        std::shared_ptr<LogicalDevice> l_device;

        std::vector<VkFramebuffer> frame_buffers;
    };
}