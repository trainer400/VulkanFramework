#pragma once

#include <window/window.h>
#include <window/windowSurface.h>
#include <devices/logicalDevice.h>
#include <devices/physicalDevice.h>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace framework
{
    struct SwapChainConfiguration
    {
        VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    };

    class SwapChain
    {
    public:
        /**
         * @brief Construct a new Swap Chain object
         */
        SwapChain(const std::shared_ptr<LogicalDevice> &lDevice, const std::shared_ptr<Window> &window, const VkSurfaceKHR &surface, const SwapChainConfiguration &config);
        ~SwapChain();

        /**
         * @brief Recreates the swap chain (usually called after a window resize)
         */
        void recreateSwapChain(const std::shared_ptr<Window> &window, const VkSurfaceKHR &surface);

        // Getters
        const VkSwapchainKHR &getSwapChain() { return swapChain; }
        const std::vector<VkImage> &getImages() { return images; }
        const std::vector<VkImageView> &getImageViews() { return imageViews; }
        const VkSurfaceFormatKHR &getFormat() { return surfaceFormat; }
        const VkPresentModeKHR &getPresentMode() { return presentMode; }
        const VkExtent2D &getExtent() { return extent; }

    private:
        /**
         * @brief Cleans the swap chain and the image views
         */
        void cleanup();

        /**
         * @brief Creates the swap chain itself (called by constructor and re-creation method)
         */
        void createSwapChain(const std::shared_ptr<Window> &window, const VkSurfaceKHR &surface);

        /**
         * @brief Creates the corresponding image views for the created images
         */
        void createImageViews();

        /**
         * @brief Chooses the best format among the supported ones from the physical device
         */
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

        /**
         * @brief Chooses the best present mode among the supported ones from the physical device.
         * It aims for VK_PRESENT_MODE_MAILBOX_KHR, which implements the commonly known "triple buffering" in vsync.
         * In case of a failure it selects the VK_PRESENT_MODE_FIFO_KHR, which is the common vsync.
         */
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

        /**
         * @brief Selects the maximum resolution available inside the created window
         */
        VkExtent2D chooseSwapContext(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window);

        VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        std::shared_ptr<LogicalDevice> lDevice;
        SwapChainSupportDetails swapChainSupport;
        SwapChainConfiguration config;

        // Set of images and views
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;

        // Set of choosen parameters for swap chain
        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR presentMode;
        VkExtent2D extent;
    };
}