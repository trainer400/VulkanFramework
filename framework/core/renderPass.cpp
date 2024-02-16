#include "renderPass.h"
#include <stdexcept>

namespace framework
{
    RenderPass::RenderPass(const std::shared_ptr<LogicalDevice> &lDevice, const VkExtent2D &extent, const VkSurfaceFormatKHR &format, DepthTestType depth)
    {
        if (lDevice == nullptr)
        {
            throw std::runtime_error("[RenderPass] Null device instance");
        }

        this->lDevice = lDevice;
        this->depth = depth;

        createRenderPass(extent, format);
    }

    RenderPass::~RenderPass()
    {
        cleanup();
    }

    void RenderPass::createRenderPass(const VkExtent2D &extent, const VkSurfaceFormatKHR &format)
    {
        // Structs to be used in case of a depth buffer active
        VkAttachmentDescription depthAttachment{};
        VkAttachmentReference depthAttachmentRef{};

        // Depth buffer
        if (depth != NONE)
        {
            VkFormat depthFormat = static_cast<VkFormat>(depth);

            // Check that the requested depth buffering is supported
            checkFormat(depthFormat,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

            // Create the depth buffer image
            createImage(extent.width, extent.height,
                        depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);

            createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, depthImageView);

            // Populate the attachment
            depthAttachment.format = depthFormat;
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkAttachmentDescription colorAttachment{};

        colorAttachment.format = format.format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        // The frame buffer is going to be cleared before a drawing operation
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // Don't care for stencils
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // The images are presented for a swap chain
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Setup subpass
        VkAttachmentReference colorAttachmentRef{};

        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        if (depth != NONE)
        {
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            subpass.pDepthStencilAttachment = &depthAttachmentRef;
        }

        // Create the subpass dependency
        VkSubpassDependency dependency{};

        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        if (depth != NONE)
        {
            // Adjust the subpass dependency
            dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }

        // Create the vector with the attachments
        std::vector<VkAttachmentDescription> attachments;
        attachments.push_back(colorAttachment);

        if (depth != NONE)
        {
            attachments.push_back(depthAttachment);
        }

        // Create the renderpass
        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        createInfo.pAttachments = attachments.data();
        createInfo.subpassCount = 1;
        createInfo.pSubpasses = &subpass;
        createInfo.dependencyCount = 1;
        createInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(lDevice->getDevice(), &createInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("[RenderPass] Impossible to create render pass instance");
        }
    }

    void RenderPass::cleanup()
    {
        // Destroy the depth buffer
        if (depth != NONE)
        {
            if (depthImageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(lDevice->getDevice(), depthImageView, nullptr);
            }

            if (depthImage != VK_NULL_HANDLE)
            {
                vkDestroyImage(lDevice->getDevice(), depthImage, nullptr);
            }

            if (depthImageMemory != VK_NULL_HANDLE)
            {
                vkFreeMemory(lDevice->getDevice(), depthImageMemory, nullptr);
            }
        }

        if (renderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(lDevice->getDevice(), renderPass, nullptr);
        }
    }

    void RenderPass::recreateRenderPass(const VkExtent2D &extent, const VkSurfaceFormatKHR &format)
    {
        // Remove the previously created one
        cleanup();

        // Recreate the render pass
        createRenderPass(extent, format);
    }

    void RenderPass::checkFormat(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        // Enumerate the format properties
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(lDevice->getPhysicalDevice()->getDevice(), format, &props);

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

    void RenderPass::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory)
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
            throw std::runtime_error("[RenderPass] Failed to create image");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(lDevice->getDevice(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(lDevice->getDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("[RenderPass] Failed to allocate image memory");
        }

        vkBindImageMemory(lDevice->getDevice(), image, imageMemory, 0);
    }

    uint32_t RenderPass::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

        throw std::runtime_error("[RenderPass] Unable to find a suitable memory type");
    }

    void RenderPass::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView &view)
    {
        VkImageViewCreateInfo createInfo{};

        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // No mipmap level nor multiple layers
        createInfo.subresourceRange.aspectMask = aspectFlags;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(lDevice->getDevice(), &createInfo, nullptr, &view) != VK_SUCCESS)
        {
            throw std::runtime_error("[RenderPass] Error creating the image views");
        }
    }
}