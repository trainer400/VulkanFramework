#include "physicalDevice.h"
#include <stdexcept>
#include <vector>
#include <set>

namespace framework
{
    PhysicalDevice::PhysicalDevice(VkInstance instance, VkSurfaceKHR surface, uint32_t index) : instance(instance), surface(surface), deviceIndex(index)
    {
        if (instance == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[PhysicalDevice] Null vulkan instance");
        }

        if (surface == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[PhysicalDevice] Null surface instance");
        }

        uint32_t devicesNumber = getDevicesNumber();

        // Check parameter validity
        if (index >= devicesNumber)
        {
            throw std::runtime_error("[PhysicalDevice] Index > #devices");
        }

        // Init the extension list
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // Vector in which insert the devices enumeration
        std::vector<VkPhysicalDevice> devices(devicesNumber);

        // Enumerate devices
        vkEnumeratePhysicalDevices(this->instance, &devicesNumber, devices.data());

        // Check if the device could work with vulkan
        if (isDeviceSuitable(devices[this->deviceIndex]))
        {
            this->pDevice = devices[this->deviceIndex];
        }
        else
        {
            throw std::runtime_error("[PhysicalDevice] Device not suited");
        }
    }

    uint32_t PhysicalDevice::getDevicesNumber()
    {
        uint32_t number;
        if (vkEnumeratePhysicalDevices(this->instance, &number, nullptr) != VK_SUCCESS)
        {
            throw std::runtime_error("[PhysicalDevice] Impossible to enumerate VK physical devices");
        }

        return number;
    }

    bool PhysicalDevice::isDeviceSuitable(VkPhysicalDevice device)
    {
        // Immediate check
        if (device == VK_NULL_HANDLE)
        {
            return false;
        }

        // Enumerate device features and properties
        VkPhysicalDeviceProperties prop;
        VkPhysicalDeviceFeatures feat;

        vkGetPhysicalDeviceProperties(device, &prop);
        vkGetPhysicalDeviceFeatures(device, &feat);

        // Check device
        bool devicePhysical = (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
        return devicePhysical && feat.geometryShader && feat.samplerAnisotropy && checkDeviceExtensionSupport(device) && checkSwapChainAdequate(device);
    }

    bool PhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        if (device == VK_NULL_HANDLE)
        {
            return false;
        }

        uint32_t extensionCount;

        // Get the number of extensions
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        // Enumerate the extensions
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // Create a set with all the required extensions
        std::set<std::string> requiredExtension(deviceExtensions.begin(), deviceExtensions.end());

        // Remove the available extensions from the required ones
        for (const VkExtensionProperties extension : availableExtensions)
        {
            requiredExtension.erase(extension.extensionName);
        }

        // If all needed extensions are supported the final set is empty
        return requiredExtension.empty();
    }

    SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(VkPhysicalDevice device)
    {
        if (device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[PhysicalDevice] Null physical device instance");
        }

        SwapChainSupportDetails details;

        // Query the capabilities of the physical device
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        // Query the formats number
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            // Size the vector to the exact number
            details.formats.resize(formatCount);

            // Assign the formats to the vector
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        // Query the presentation modes number
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            // Size the vector to the exact number
            details.presentModes.resize(presentModeCount);

            // Assign the present modes to the vector
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    bool PhysicalDevice::checkSwapChainAdequate(VkPhysicalDevice device)
    {
        if (device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[PhysicalDevice] Null physical device instance");
        }

        // Query the swap chain support details
        swapChainSupport = querySwapChainSupport(device);

        // Check if there is something in formats and presentation modes
        bool result = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

        return result;
    }
}