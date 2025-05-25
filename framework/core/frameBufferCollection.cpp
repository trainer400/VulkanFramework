#include "frameBufferCollection.h"
#include <stdexcept>

namespace framework
{
    FrameBufferCollection::FrameBufferCollection(const std::shared_ptr<LogicalDevice> &l_device,
                                                 const std::vector<VkImageView> &image_views, const VkExtent2D &extent,
                                                 const DepthTestType &dept_test_type, const VkImageView &depth_image_view, const VkRenderPass &render_pass)
    {
        if (l_device == nullptr)
        {
            throw std::runtime_error("[FrameBufferCollection] Null device instance");
        }

        if (render_pass == nullptr)
        {
            throw std::runtime_error("[FrameBufferCollection] Null Render Pass instance");
        }

        this->l_device = l_device;

        createFrameBuffer(image_views, extent, dept_test_type, depth_image_view, render_pass);
    }

    FrameBufferCollection::~FrameBufferCollection()
    {
        cleanup();
    }

    void FrameBufferCollection::createFrameBuffer(const std::vector<VkImageView> &image_views, const VkExtent2D &extent,
                                                  const DepthTestType &dept_test_type, const VkImageView &depth_image_view, const VkRenderPass &render_pass)
    {
        // Get the images pointer
        size_t images_size = image_views.size();

        frame_buffers.clear();
        frame_buffers.resize(images_size);

        for (size_t i = 0; i < images_size; i++)
        {
            std::vector<VkImageView> attachments;
            attachments.push_back(image_views[i]);

            // Add the depth buffer if necessary
            if (dept_test_type != NONE && depth_image_view != VK_NULL_HANDLE)
            {
                attachments.push_back(depth_image_view);
            }

            VkFramebufferCreateInfo create_info{};

            create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            create_info.renderPass = render_pass;
            create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
            create_info.pAttachments = attachments.data();
            create_info.width = extent.width;
            create_info.height = extent.height;
            create_info.layers = 1;

            if (vkCreateFramebuffer(l_device->getDevice(), &create_info, nullptr, &frame_buffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("[FrameBufferCollection] Impossible to create a frame buffer");
            }
        }
    }

    void FrameBufferCollection::recreateFrameBuffer(const std::vector<VkImageView> &image_views, const VkExtent2D &extent,
                                                    const DepthTestType &dept_test_type, const VkImageView &depth_image_view, const VkRenderPass &render_pass)
    {
        // Clean the previous
        cleanup();

        // Create the new frame buffer
        createFrameBuffer(image_views, extent, dept_test_type, depth_image_view, render_pass);
    }

    void FrameBufferCollection::cleanup()
    {
        for (VkFramebuffer buffer : frame_buffers)
        {
            if (l_device->getDevice() != VK_NULL_HANDLE && buffer != VK_NULL_HANDLE)
            {
                vkDestroyFramebuffer(l_device->getDevice(), buffer, nullptr);
            }
        }
    }
}