#include "swapChain.h"
#include <stdexcept>
#include <limits>
#include <algorithm>

namespace framework
{
    SwapChain::SwapChain(const std::shared_ptr<LogicalDevice> &lDevice, const std::shared_ptr<Window> &window, const VkSurfaceKHR &surface, const SwapChainConfiguration &config)
    {
        if (lDevice == nullptr)
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

        this->lDevice = lDevice;
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
        if (lDevice->getDevice() != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(lDevice->getDevice(), swapChain, nullptr);
        }

        // Delete the image views
        for (VkImageView view : imageViews)
        {
            vkDestroyImageView(lDevice->getDevice(), view, nullptr);
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
        swapChainSupport = lDevice->getPhysicalDevice()->getSwapChainSupportDetails();

        // Get the best features
        surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        extent = chooseSwapContext(swapChainSupport.capabilities, window->getWindow());

        // Configure the swap chain with retrieved data
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        // Check if the device can handle that
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        // Populate the configuration struct
        VkSwapchainCreateInfoKHR createInfo{};

        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Configure family indices
        QueueFamilyIndices indices = lDevice->findQueueFamilies(surface);

        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
        if (queueFamilyIndices[0] != queueFamilyIndices[1])
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        // TODO to consider for window resizing
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(lDevice->getDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("[SwapChain] Error creating the swap chain");
        }

        // Get the images objects
        images.clear();
        vkGetSwapchainImagesKHR(lDevice->getDevice(), swapChain, &imageCount, nullptr);
        images.resize(imageCount);
        vkGetSwapchainImagesKHR(lDevice->getDevice(), swapChain, &imageCount, images.data());
    }

    void SwapChain::createImageViews()
    {
        // Resize the vector depending on the number of images
        imageViews.clear();
        imageViews.resize(images.size());

        // For every image create the corresponding image view
        for (int i = 0; i < images.size(); i++)
        {
            VkImageViewCreateInfo createInfo{};

            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = images[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = surfaceFormat.format;

            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // No mipmap level nor multiple layers
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(lDevice->getDevice(), &createInfo, nullptr, &imageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("[SwapChain] Error creating the image views");
            }
        }
    }

    VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const VkSurfaceFormatKHR &format : availableFormats)
        {
            if (format.format == config.format && format.colorSpace == config.colorSpace)
            {
                return format;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const VkPresentModeKHR mode : availablePresentModes)
        {
            if (mode == config.presentMode)
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