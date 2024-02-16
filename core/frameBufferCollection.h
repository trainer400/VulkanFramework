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
        FrameBufferCollection(const std::shared_ptr<LogicalDevice> &lDevice,
                              const std::vector<VkImageView> &imageViews, const VkExtent2D &extent,
                              const DepthTestType &depthTestType, const VkImageView &depthImageView, const VkRenderPass &renderPass);
        ~FrameBufferCollection();

        /**
         * @brief Recreates the frame buffer (usually called after window resize)
         */
        void recreateFrameBuffer(const std::vector<VkImageView> &imageViews, const VkExtent2D &extent,
                                 const DepthTestType &depthTestType, const VkImageView &depthImageView, const VkRenderPass &renderPass);

        // Getter
        const std::vector<VkFramebuffer> &getFrameBuffers() { return frameBuffers; }

    private:
        /**
         * @brief Creates the actual frame buffer. It is called by the constructor and by the
         * frame buffer re-creation method.
         */
        void createFrameBuffer(const std::vector<VkImageView> &imageViews, const VkExtent2D &extent,
                               const DepthTestType &depthTestType, const VkImageView &depthImageView, const VkRenderPass &renderPass);

        /**
         * @brief Cleans everything and removes the frame buffers
         */
        void cleanup();

        std::shared_ptr<LogicalDevice> lDevice;

        std::vector<VkFramebuffer> frameBuffers;
    };
}