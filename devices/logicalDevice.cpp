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

        this->pDevice = std::move(p);

        // Vulkan allows for queue priorities expressed from 0 to 1
        float queuePriority = 1.0;

        // Find the indices for appropriate queues if present
        QueueFamilyIndices indices = findQueueFamilies(surface);

        // Create the set of queue indices
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        // Create the vector of queue create info structs
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        // Gather the device extensions
        std::vector<const char *> deviceExtensions = pDevice->getDeviceExtensions();

        // Configuration structs. Need to use the iterator mode due to the "set" struct
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;

            // Add the struct to the vector
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.geometryShader = VK_TRUE;
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceFeatures.fillModeNonSolid = VK_TRUE; // Allows for wireframe

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.enabledLayerCount = 0;

        // Create the device with defined features
        if (vkCreateDevice(pDevice->getDevice(), &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("[LogicalDevice] Failed to create a logical device");
        }

        // Retrieve the created queue
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
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
        vkGetPhysicalDeviceQueueFamilyProperties(pDevice->getDevice(), &count, nullptr);

        // Enumerate the families
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(pDevice->getDevice(), &count, families.data());

        // Check the family indices depending on what is present
        for (uint32_t i = 0; i < count; i++)
        {
            // IN CASE SOME QUEUE NEEDS TO BE ADDED, ADD IT HERE
            if (families.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                result.graphicsFamily = i;
            }

            // Check window surface compatibility
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(pDevice->getDevice(), i, surface, &presentSupport);

            if (presentSupport)
            {
                result.presentFamily = i;
            }
        }
        return result;
    }
}