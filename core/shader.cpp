#include "shader.h"
#include <fstream>
namespace framework
{
    Shader::Shader(const std::shared_ptr<LogicalDevice> &lDevice, const char *filename, ShaderType type) : type(type)
    {
        if (filename == nullptr)
        {
            throw std::runtime_error("[Shader] Null filename");
        }

        if (lDevice == nullptr)
        {
            throw std::runtime_error("[Shader] Null device instance");
        }

        this->lDevice = lDevice;

        // Open the file. Ate reads the file from the end (to retrieve the dimensions)
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        // Check the file is actually open
        if (!file.is_open())
        {
            throw std::runtime_error("[Shader] File open failure");
        }

        size_t fileSize = (size_t)file.tellg();

        // Size the buffer as the dimension of the file
        fileBuffer.resize(fileSize);

        // Position the pointer to the beginning of the file
        file.seekg(0);

        // Read the entire buffer
        file.read(fileBuffer.data(), fileSize);

        VkShaderModuleCreateInfo createInfo{};

        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = fileBuffer.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(fileBuffer.data());

        if (vkCreateShaderModule(lDevice->getDevice(), &createInfo, nullptr, &shader) != VK_SUCCESS)
        {
            throw std::runtime_error("[Shader] Impossible to create the shader");
        }

        file.close();
    }

    Shader::~Shader()
    {
        vkDestroyShaderModule(lDevice->getDevice(), shader, nullptr);
    }

    VkShaderStageFlagBits Shader::getShaderStage()
    {
        // Select the function depending on the user passed type
        switch (type)
        {
        case VERTEX:
            return VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case TESSELLATION:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            break;
        case GEOMETRY:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
            break;
        case FRAGMENT:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
        default:
            throw std::runtime_error("[Shader] Shader type not listed");
        }
    }
}