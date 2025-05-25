#include "vulkan.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <set>
#include <cstring>
#include <iostream>

namespace framework
{
    Vulkan::Vulkan(const char *application_name, const char *engine_name, const std::vector<const char *> &added_extensions, bool enable_layers)
    {
        if (application_name == nullptr || engine_name == nullptr)
        {
            throw std::runtime_error("[Vulkan] Nullptrs in application and engine names");
        }

        const char **glfw_extensions;
        uint32_t glfw_extension_count = 0;

        // Gather the glfw info
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        if (glfw_extensions == nullptr)
        {
            throw std::runtime_error("[Vulkan] Impossible to enumerate instance extensions");
        }

        // Merge the extension names
        char **extensions;
        extensions = new char *[glfw_extension_count + added_extensions.size()];

        for (int i = 0; i < glfw_extension_count; i++)
        {
            extensions[i] = new char[strlen(glfw_extensions[i]) + 1];
            strcpy(extensions[i], glfw_extensions[i]);
        }

        for (int i = 0; i < added_extensions.size(); i++)
        {
            extensions[glfw_extension_count + i] = new char[strlen(added_extensions[i]) + 1];
            strcpy(extensions[glfw_extension_count + i], added_extensions[i]);
        }

        // Setup the structure to instantiate vulkan
        VkApplicationInfo application_info{};
        application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        application_info.pApplicationName = application_name;
        application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        application_info.pEngineName = engine_name;
        application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        application_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &application_info;
        create_info.enabledExtensionCount = glfw_extension_count + added_extensions.size();
        create_info.ppEnabledExtensionNames = extensions;
        create_info.enabledLayerCount = 0;

        // Create validation layers
        if (enable_layers)
        {
            // Enumerate valiation layers
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers;
            availableLayers.resize(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            // Check validation layers support
            bool layer_found = false;
            for (const char *layer_name : validation_layers)
            {
                layer_found = false;
                for (const VkLayerProperties properties : availableLayers)
                {
                    if (strcmp(layer_name, properties.layerName) == 0)
                    {
                        layer_found = true;

                        // Found the layer
                        break;
                    }
                }

                // In case of layer not found break
                if (!layer_found)
                {
                    throw std::runtime_error("[Vulkan] Not compatible validation layer found");
                }
            }

            // Set the validation layers
            create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();
        }

        // Create vulkan instance
        if (vkCreateInstance(&create_info, nullptr, &this->instance) != VK_SUCCESS)
        {
            throw std::runtime_error("[Vulkan] Impossible to create VK instance");
        }

        // After instance creation free the extensions
        for (int i = 0; i < glfw_extension_count + added_extensions.size(); i++)
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