#include "vulkan.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <set>
#include <cstring>
#include <iostream>

namespace framework
{
    Vulkan::Vulkan(const char *applicationName, const char *engineName, const std::vector<const char *> &addedExtensions, bool enableLayers)
    {
        if (applicationName == nullptr || engineName == nullptr)
        {
            throw std::runtime_error("[Vulkan] Nullptrs in application and engine names");
        }

        const char **glfwExtensions;
        uint32_t glfwExtensionCount = 0;

        // Gather the glfw info
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        if (glfwExtensions == nullptr)
        {
            throw std::runtime_error("[Vulkan] Impossible to enumerate instance extensions");
        }

        // Merge the extension names
        char **extensions;
        extensions = new char *[glfwExtensionCount + addedExtensions.size()];

        for (int i = 0; i < glfwExtensionCount; i++)
        {
            extensions[i] = new char[strlen(glfwExtensions[i]) + 1];
            strcpy(extensions[i], glfwExtensions[i]);
        }

        for (int i = 0; i < addedExtensions.size(); i++)
        {
            extensions[glfwExtensionCount + i] = new char[strlen(addedExtensions[i]) + 1];
            strcpy(extensions[glfwExtensionCount + i], addedExtensions[i]);
        }

        // Setup the structure to instantiate vulkan
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = applicationName;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = engineName;
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = glfwExtensionCount + addedExtensions.size();
        createInfo.ppEnabledExtensionNames = extensions;
        createInfo.enabledLayerCount = 0;

        // Create validation layers
        if (enableLayers)
        {
            // Enumerate valiation layers
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers;
            availableLayers.resize(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            // Check validation layers support
            bool layerFound = false;
            for (const char *layerName : validationLayers)
            {
                layerFound = false;
                for (const VkLayerProperties properties : availableLayers)
                {
                    if (strcmp(layerName, properties.layerName) == 0)
                    {
                        layerFound = true;

                        // Found the layer
                        break;
                    }
                }

                // In case of layer not found break
                if (!layerFound)
                {
                    throw std::runtime_error("[Vulkan] Not compatible validation layer found");
                }
            }

            // Set the validation layers
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }

        // Create vulkan instance
        if (vkCreateInstance(&createInfo, nullptr, &this->instance) != VK_SUCCESS)
        {
            throw std::runtime_error("[Vulkan] Impossible to create VK instance");
        }

        // After instance creation free the extensions
        for (int i = 0; i < glfwExtensionCount + addedExtensions.size(); i++)
        {
            delete (extensions[i]);
        }
        delete extensions;
    }

    Vulkan::~Vulkan()
    {
        vkDestroyInstance(instance, nullptr);
    }
}