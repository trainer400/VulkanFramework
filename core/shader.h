#pragma once

#include <devices/logicalDevice.h>

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace framework
{
    enum ShaderType : uint8_t
    {
        VERTEX = 0,
        TESSELLATION,
        GEOMETRY,
        FRAGMENT,
    };

    class Shader
    {
    public:
        Shader(const std::shared_ptr<LogicalDevice> &lDevice, const char *filename, ShaderType type);
        ~Shader();

        // Getters
        const VkShaderModule &getShader() { return shader; }
        VkShaderStageFlagBits getShaderStage();

    private:
        std::shared_ptr<LogicalDevice> lDevice;

        VkShaderModule shader = VK_NULL_HANDLE;
        std::vector<char> fileBuffer;

        ShaderType type;
    };
}