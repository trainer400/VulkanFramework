#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace framework
{
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };
    class PhysicalDevice
    {
    public:
        /**
         * @brief Construct a new Physical Device object
         *
         * @param instance Vulkan instance
         * @param surface The window surface that the device has to support
         * @param index Index of device to choose
         */
        PhysicalDevice(VkInstance instance, VkSurfaceKHR surface, uint32_t index);

        // Getters
        inline const VkPhysicalDevice &getDevice() { return p_device; }
        const std::vector<const char *> &getDeviceExtensions() { return device_extensions; }
        SwapChainSupportDetails getSwapChainSupportDetails() { return querySwapChainSupport(p_device); }

    private:
        /**
         * @brief Get the Devices Number
         *
         * @return uint32_t Number of physical available devices
         */
        uint32_t getDevicesNumber();

        /**
         * @brief Checks if the device supports the geometry shader
         */
        bool isDeviceSuitable(VkPhysicalDevice device);

        /**
         * @brief Checks if the device supports the const list of extensions
         */
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        /**
         * @brief Queries the swap chain support details that the physical device has
         */
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        /**
         * @brief Checks if the device supports the swapchain with the choosen window surface
         */
        bool checkSwapChainAdequate(VkPhysicalDevice device);

        VkPhysicalDevice p_device = VK_NULL_HANDLE;
        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        uint32_t device_index;

        // Vector of needed supported extensions for the physical device
        std::vector<const char *> device_extensions;

        // List of details that the physical device supports
        SwapChainSupportDetails swap_chain_support;
    };
}