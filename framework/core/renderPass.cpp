#include "renderPass.h"
#include <stdexcept>

namespace framework
{
    RenderPass::RenderPass(const std::shared_ptr<LogicalDevice> &l_device, const VkExtent2D &extent, const VkSurfaceFormatKHR &format, DepthTestType depth)
        : RenderPass(l_device, extent, format, RenderPassConfiguration{depth, VK_SAMPLE_COUNT_1_BIT})
    {
    }

    RenderPass::RenderPass(const std::shared_ptr<LogicalDevice> &l_device, const VkExtent2D &extent, const VkSurfaceFormatKHR &format, const RenderPassConfiguration &config)
    {
        if (l_device == nullptr)
        {
            throw std::runtime_error("[RenderPass] Null device instance");
        }

        this->l_device = l_device;
        this->config = config;

        createRenderPass(extent, format);
    }

    RenderPass::~RenderPass()
    {
        cleanup();
    }

    void RenderPass::createRenderPass(const VkExtent2D &extent, const VkSurfaceFormatKHR &format)
    {
        if (config.sample_count != VK_SAMPLE_COUNT_1_BIT)
        {
            // Check that the requested multisampling count is supported
            checkSampleCount(format.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

            // Create the color image
            createImage(extent.width, extent.height,
                        format.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, color_image, color_image_memory);

            createImageView(color_image, format.format, VK_IMAGE_ASPECT_COLOR_BIT, color_image_view);
        }

        // Structs to be used in case of a depth buffer active
        VkAttachmentDescription depth_attachment{};
        VkAttachmentReference depth_attachment_ref{};

        // Depth buffer
        if (config.depth_test_type != NONE)
        {
            VkFormat depthFormat = static_cast<VkFormat>(config.depth_test_type);

            // Check that the requested depth buffering is supported
            checkFormat(depthFormat,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

            // Create the depth buffer image
            createImage(extent.width, extent.height,
                        depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_image, depth_image_memory);

            createImageView(depth_image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, depth_image_view);

            // Populate the attachment
            depth_attachment.format = depthFormat;
            depth_attachment.samples = config.sample_count;
            depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkAttachmentDescription color_attachment{};

        color_attachment.format = format.format;
        color_attachment.samples = config.sample_count;

        // The frame buffer is going to be cleared before a drawing operation
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = config.sample_count != VK_SAMPLE_COUNT_1_BIT ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;

        // Don't care for stencils
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // Multisampled color is resolved into the swap chain image
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = config.sample_count != VK_SAMPLE_COUNT_1_BIT ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Setup subpass
        VkAttachmentReference color_attachment_ref{};

        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;

        if (config.depth_test_type != NONE)
        {
            depth_attachment_ref.attachment = 1;
            depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            subpass.pDepthStencilAttachment = &depth_attachment_ref;
        }

        // Create the subpass dependency
        VkSubpassDependency dependency{};

        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        if (config.depth_test_type != NONE)
        {
            // Adjust the subpass dependency
            dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }

        // Create the vector with the attachments
        std::vector<VkAttachmentDescription> attachments;
        attachments.push_back(color_attachment);

        if (config.depth_test_type != NONE)
        {
            attachments.push_back(depth_attachment);
        }

        VkAttachmentReference resolve_attachment_ref{};
        if (config.sample_count != VK_SAMPLE_COUNT_1_BIT)
        {
            VkAttachmentDescription resolve_attachment = color_attachment;
            resolve_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            resolve_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolve_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            resolve_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            resolve_attachment_ref.attachment = static_cast<uint32_t>(attachments.size());
            resolve_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            subpass.pResolveAttachments = &resolve_attachment_ref;
            attachments.push_back(resolve_attachment);
        }

        // Create the renderpass
        VkRenderPassCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        create_info.pAttachments = attachments.data();
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass;
        create_info.dependencyCount = 1;
        create_info.pDependencies = &dependency;

        if (vkCreateRenderPass(l_device->getDevice(), &create_info, nullptr, &render_pass) != VK_SUCCESS)
        {
            throw std::runtime_error("[RenderPass] Impossible to create render pass instance");
        }
    }

    void RenderPass::cleanup()
    {
        if (color_image_view != VK_NULL_HANDLE)
        {
            vkDestroyImageView(l_device->getDevice(), color_image_view, nullptr);
            color_image_view = VK_NULL_HANDLE;
        }

        if (color_image != VK_NULL_HANDLE)
        {
            vkDestroyImage(l_device->getDevice(), color_image, nullptr);
            color_image = VK_NULL_HANDLE;
        }

        if (color_image_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(l_device->getDevice(), color_image_memory, nullptr);
            color_image_memory = VK_NULL_HANDLE;
        }

        // Destroy the depth buffer
        if (config.depth_test_type != NONE)
        {
            if (depth_image_view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(l_device->getDevice(), depth_image_view, nullptr);
                depth_image_view = VK_NULL_HANDLE;
            }

            if (depth_image != VK_NULL_HANDLE)
            {
                vkDestroyImage(l_device->getDevice(), depth_image, nullptr);
                depth_image = VK_NULL_HANDLE;
            }

            if (depth_image_memory != VK_NULL_HANDLE)
            {
                vkFreeMemory(l_device->getDevice(), depth_image_memory, nullptr);
                depth_image_memory = VK_NULL_HANDLE;
            }
        }

        if (render_pass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(l_device->getDevice(), render_pass, nullptr);
            render_pass = VK_NULL_HANDLE;
        }
    }

    void RenderPass::recreateRenderPass(const VkExtent2D &extent, const VkSurfaceFormatKHR &format)
    {
        // Remove the previously created one
        cleanup();

        // Recreate the render pass
        createRenderPass(extent, format);
    }

    void RenderPass::begin(const VkCommandBuffer &cmd_buffer, const VkFramebuffer &frame_buffer, const VkExtent2D &extent, const VkClearValue &clear_color)
    {
        // Start the render pass
        VkRenderPassBeginInfo render_pass_info{};

        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = render_pass;
        render_pass_info.framebuffer = frame_buffer;
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = extent;

        // Add the user specified clear color
        std::vector<VkClearValue> clear_values;
        clear_values.push_back(clear_color);
        if (config.depth_test_type != NONE)
        {
            clear_values.push_back({1.0f, 0.0f}); // Depth and stencil
        }

        render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_info.pClearValues = clear_values.data();

        vkCmdBeginRenderPass(cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void RenderPass::end(const VkCommandBuffer &cmd_buffer)
    {
        // End the render pass
        vkCmdEndRenderPass(cmd_buffer);
    }

    void RenderPass::checkFormat(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        // Enumerate the format properties
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(l_device->getPhysicalDevice()->getDevice(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            // The device supports the format in tiling linear image
            return;
        }

        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            // The device supports the format in tiling optimal image
            return;
        }

        throw std::runtime_error("[RenderPass] Failed to find supported format");
    }

    void RenderPass::checkSampleCount(VkFormat format, VkImageUsageFlags usage)
    {
        VkImageFormatProperties properties;
        if (vkGetPhysicalDeviceImageFormatProperties(l_device->getPhysicalDevice()->getDevice(), format,
                                                    VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &properties) != VK_SUCCESS ||
            (properties.sampleCounts & config.sample_count) == 0)
        {
            throw std::runtime_error("[RenderPass] Unsupported sample count for image format");
        }
    }

    void RenderPass::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory)
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
        image_info.samples = config.sample_count;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(l_device->getDevice(), &image_info, nullptr, &image) != VK_SUCCESS)
        {
            throw std::runtime_error("[RenderPass] Failed to create image");
        }

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(l_device->getDevice(), image, &mem_requirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = mem_requirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(mem_requirements.memoryTypeBits, properties);

        if (vkAllocateMemory(l_device->getDevice(), &allocInfo, nullptr, &image_memory) != VK_SUCCESS)
        {
            throw std::runtime_error("[RenderPass] Failed to allocate image memory");
        }

        vkBindImageMemory(l_device->getDevice(), image, image_memory, 0);
    }

    uint32_t RenderPass::findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
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

        throw std::runtime_error("[RenderPass] Unable to find a suitable memory type");
    }

    void RenderPass::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView &view)
    {
        VkImageViewCreateInfo create_info{};

        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = image;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = format;

        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // No mipmap level nor multiple layers
        create_info.subresourceRange.aspectMask = aspect_flags;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(l_device->getDevice(), &create_info, nullptr, &view) != VK_SUCCESS)
        {
            throw std::runtime_error("[RenderPass] Error creating the image views");
        }
    }
}