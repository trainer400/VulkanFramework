#include "frameBufferCollection.h"
#include <stdexcept>

namespace framework
{
    FrameBufferCollection::FrameBufferCollection(const std::shared_ptr<LogicalDevice> &lDevice,
                                                 const std::vector<VkImageView> &imageViews, const VkExtent2D &extent,
                                                 const DepthTestType &depthTestType, const VkImageView &depthImageView, const VkRenderPass &renderPass)
    {
        if (lDevice == nullptr)
        {
            throw std::runtime_error("[FrameBufferCollection] Null device instance");
        }

        if (renderPass == nullptr)
        {
            throw std::runtime_error("[FrameBufferCollection] Null Render Pass instance");
        }

        this->lDevice = lDevice;

        createFrameBuffer(imageViews, extent, depthTestType, depthImageView, renderPass);
    }

    FrameBufferCollection::~FrameBufferCollection()
    {
        cleanup();
    }

    void FrameBufferCollection::createFrameBuffer(const std::vector<VkImageView> &imageViews, const VkExtent2D &extent,
                                                  const DepthTestType &depthTestType, const VkImageView &depthImageView, const VkRenderPass &renderPass)
    {
        // Get the images pointer
        size_t imagesSize = imageViews.size();

        frameBuffers.clear();
        frameBuffers.resize(imagesSize);

        for (size_t i = 0; i < imagesSize; i++)
        {
            std::vector<VkImageView> attachments;
            attachments.push_back(imageViews[i]);

            // Add the depth buffer if necessary
            if (depthTestType != NONE && depthImageView != VK_NULL_HANDLE)
            {
                attachments.push_back(depthImageView);
            }

            VkFramebufferCreateInfo createInfo{};

            createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            createInfo.renderPass = renderPass;
            createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            createInfo.pAttachments = attachments.data();
            createInfo.width = extent.width;
            createInfo.height = extent.height;
            createInfo.layers = 1;

            if (vkCreateFramebuffer(lDevice->getDevice(), &createInfo, nullptr, &frameBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("[FrameBufferCollection] Impossible to create a frame buffer");
            }
        }
    }

    void FrameBufferCollection::recreateFrameBuffer(const std::vector<VkImageView> &imageViews, const VkExtent2D &extent,
                                                    const DepthTestType &depthTestType, const VkImageView &depthImageView, const VkRenderPass &renderPass)
    {
        // Clean the previous
        cleanup();

        // Create the new frame buffer
        createFrameBuffer(imageViews, extent, depthTestType, depthImageView, renderPass);
    }

    void FrameBufferCollection::cleanup()
    {
        for (VkFramebuffer buffer : frameBuffers)
        {
            if (lDevice->getDevice() != VK_NULL_HANDLE && buffer != VK_NULL_HANDLE)
            {
                vkDestroyFramebuffer(lDevice->getDevice(), buffer, nullptr);
            }
        }
    }
}