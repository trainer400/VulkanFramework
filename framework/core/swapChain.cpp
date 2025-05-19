#include "swapChain.h"
#include <stdexcept>
#include <limits>
#include <algorithm>

namespace framework
{
    SwapChain::SwapChain(const std::shared_ptr<LogicalDevice> &l_device, const std::shared_ptr<Window> &window, const VkSurfaceKHR &surface, const SwapChainConfiguration &config)
    {
        if (l_device == nullptr)
        {
            throw std::runtime_error("[SwapChain] Null logical device");
        }

        if (surface == nullptr)
        {
            throw std::runtime_error("[SwapChain] Null surface instance");
        }

        if (window == nullptr)
        {
            throw std::runtime_error("[SwapChain] Null window instance");
        }

        this->l_device = l_device;
        this->config = config;

        // Create the swap chain
        createSwapChain(window, surface);

        // Create the corresponding image views
        createImageViews();
    }

    SwapChain::~SwapChain()
    {
        cleanup();
    }

    void SwapChain::cleanup()
    {
        if (l_device->getDevice() != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(l_device->getDevice(), swap_chain, nullptr);
        }

        // Delete the image views
        for (VkImageView view : image_views)
        {
            vkDestroyImageView(l_device->getDevice(), view, nullptr);
        }
    }

    void SwapChain::recreateSwapChain(const std::shared_ptr<Window> &window, const VkSurfaceKHR &surface)
    {
        // Clean the swap chain and image views
        cleanup();

        // Create the swap chain
        createSwapChain(window, surface);

        // Create the corresponding image views
        createImageViews();
    }

    void SwapChain::createSwapChain(const std::shared_ptr<Window> &window, const VkSurfaceKHR &surface)
    {
        // Get the support details
        swap_chain_support = l_device->getPhysicalDevice()->getSwapChainSupportDetails();

        // Get the best features
        surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
        present_mode = chooseSwapPresentMode(swap_chain_support.presentModes);
        extent = chooseSwapContext(swap_chain_support.capabilities, window->getWindow());

        // Configure the swap chain with retrieved data
        uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

        // Check if the device can handle that
        if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
        {
            image_count = swap_chain_support.capabilities.maxImageCount;
        }

        // Populate the configuration struct
        VkSwapchainCreateInfoKHR create_info{};

        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface;
        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Configure family indices
        QueueFamilyIndices indices = l_device->findQueueFamilies(surface);

        uint32_t queue_family_indices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
        if (queue_family_indices[0] != queue_family_indices[1])
        {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = queue_family_indices;
        }
        else
        {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices = nullptr;
        }

        create_info.preTransform = swap_chain_support.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;
        // TODO to consider for window resizing
        create_info.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(l_device->getDevice(), &create_info, nullptr, &swap_chain) != VK_SUCCESS)
        {
            throw std::runtime_error("[SwapChain] Error creating the swap chain");
        }

        // Get the images objects
        images.clear();
        vkGetSwapchainImagesKHR(l_device->getDevice(), swap_chain, &image_count, nullptr);
        images.resize(image_count);
        vkGetSwapchainImagesKHR(l_device->getDevice(), swap_chain, &image_count, images.data());
    }

    void SwapChain::createImageViews()
    {
        // Resize the vector depending on the number of images
        image_views.clear();
        image_views.resize(images.size());

        // For every image create the corresponding image view
        for (int i = 0; i < images.size(); i++)
        {
            VkImageViewCreateInfo create_info{};

            create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image = images[i];
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format = surface_format.format;

            create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // No mipmap level nor multiple layers
            create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.baseMipLevel = 0;
            create_info.subresourceRange.levelCount = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount = 1;

            if (vkCreateImageView(l_device->getDevice(), &create_info, nullptr, &image_views[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("[SwapChain] Error creating the image views");
            }
        }
    }

    VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available_formats)
    {
        for (const VkSurfaceFormatKHR &format : available_formats)
        {
            if (format.format == config.format && format.colorSpace == config.color_space)
            {
                return format;
            }
        }

        return available_formats[0];
    }

    VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available_present_modes)
    {
        for (const VkPresentModeKHR mode : available_present_modes)
        {
            if (mode == config.present_mode)
            {
                return mode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D SwapChain::chooseSwapContext(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }

        // Get the frame buffer size from the window
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D result = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)};

        // Clamp the width and height depending on the maximum capabilities
        result.width = std::clamp(result.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        result.height = std::clamp(result.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return result;
    }
}