#include "logicalDevice.h"

#include <stdexcept>
#include <set>

namespace framework
{
    LogicalDevice::LogicalDevice(std::unique_ptr<PhysicalDevice> p, const VkSurfaceKHR &surface)
    {
        // Check that selectDevice has been called
        if (p == nullptr)
        {
            throw std::runtime_error("[LogicalDevice] Null physical device instance");
        }

        this->p_device = std::move(p);

        // Vulkan allows for queue priorities expressed from 0 to 1
        float queue_priority = 1.0;

        // Find the indices for appropriate queues if present
        QueueFamilyIndices indices = findQueueFamilies(surface);

        // Create the set of queue indices
        std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(), indices.present_family.value()};

        // Create the vector of queue create info structs
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

        // Gather the device extensions
        std::vector<const char *> device_extensions = p_device->getDeviceExtensions();

        // Configuration structs. Need to use the iterator mode due to the "set" struct
        for (uint32_t queue_family : unique_queue_families)
        {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_family;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;

            // Add the struct to the vector
            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features{};
        device_features.geometryShader = VK_TRUE;
        device_features.samplerAnisotropy = VK_TRUE;
        device_features.fillModeNonSolid = VK_TRUE; // Allows for wireframe

        VkDeviceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pEnabledFeatures = &device_features;
        create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
        create_info.ppEnabledExtensionNames = device_extensions.data();
        create_info.enabledLayerCount = 0;

        // Create the device with defined features
        if (vkCreateDevice(p_device->getDevice(), &create_info, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("[LogicalDevice] Failed to create a logical device");
        }

        // Retrieve the created queue
        vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
        vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
    }

    QueueFamilyIndices LogicalDevice::findQueueFamilies(const VkSurfaceKHR &surface)
    {
        if (surface == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[LogicalDevice] Surface null handle");
        }

        QueueFamilyIndices result;
        uint32_t count = 0;

        // Get the number of queues
        vkGetPhysicalDeviceQueueFamilyProperties(p_device->getDevice(), &count, nullptr);

        // Enumerate the families
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(p_device->getDevice(), &count, families.data());

        // Check the family indices depending on what is present
        for (uint32_t i = 0; i < count; i++)
        {
            // IN CASE SOME QUEUE NEEDS TO BE ADDED, ADD IT HERE
            if (families.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                result.graphics_family = i;
            }

            // Check window surface compatibility
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(p_device->getDevice(), i, surface, &present_support);

            if (present_support)
            {
                result.present_family = i;
            }
        }
        return result;
    }
}