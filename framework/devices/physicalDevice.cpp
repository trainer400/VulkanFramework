#include "physicalDevice.h"
#include <stdexcept>
#include <vector>
#include <set>

namespace framework
{
    PhysicalDevice::PhysicalDevice(VkInstance instance, VkSurfaceKHR surface, uint32_t index) : instance(instance), surface(surface), device_index(index)
    {
        if (instance == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[PhysicalDevice] Null vulkan instance");
        }

        if (surface == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[PhysicalDevice] Null surface instance");
        }

        uint32_t devices_number = getDevicesNumber();

        // Check parameter validity
        if (index >= devices_number)
        {
            throw std::runtime_error("[PhysicalDevice] Index > #devices");
        }

        // Init the extension list
        device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // Vector in which insert the devices enumeration
        std::vector<VkPhysicalDevice> devices(devices_number);

        // Enumerate devices
        vkEnumeratePhysicalDevices(this->instance, &devices_number, devices.data());

        // Check if the device could work with vulkan
        if (isDeviceSuitable(devices[this->device_index]))
        {
            this->p_device = devices[this->device_index];
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
        bool device_physical = (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
        return device_physical && feat.geometryShader && feat.samplerAnisotropy && checkDeviceExtensionSupport(device) && checkSwapChainAdequate(device);
    }

    bool PhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        if (device == VK_NULL_HANDLE)
        {
            return false;
        }

        uint32_t extension_count;

        // Get the number of extensions
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

        // Enumerate the extensions
        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

        // Create a set with all the required extensions
        std::set<std::string> required_extension(device_extensions.begin(), device_extensions.end());

        // Remove the available extensions from the required ones
        for (const VkExtensionProperties extension : available_extensions)
        {
            required_extension.erase(extension.extensionName);
        }

        // If all needed extensions are supported the final set is empty
        return required_extension.empty();
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
        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

        if (format_count != 0)
        {
            // Size the vector to the exact number
            details.formats.resize(format_count);

            // Assign the formats to the vector
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
        }

        // Query the presentation modes number
        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

        if (present_mode_count != 0)
        {
            // Size the vector to the exact number
            details.present_modes.resize(present_mode_count);

            // Assign the present modes to the vector
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
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
        swap_chain_support = querySwapChainSupport(device);

        // Check if there is something in formats and presentation modes
        bool result = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();

        return result;
    }
}