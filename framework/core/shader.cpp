#include "shader.h"
#include <fstream>
namespace framework
{
    Shader::Shader(const std::shared_ptr<LogicalDevice> &l_device, const char *filename, ShaderType type) : type(type)
    {
        if (filename == nullptr)
        {
            throw std::runtime_error("[Shader] Null filename");
        }

        if (l_device == nullptr)
        {
            throw std::runtime_error("[Shader] Null device instance");
        }

        this->l_device = l_device;

        // Open the file. Ate reads the file from the end (to retrieve the dimensions)
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        // Check the file is actually open
        if (!file.is_open())
        {
            throw std::runtime_error("[Shader] File open failure");
        }

        size_t file_size = (size_t)file.tellg();

        // Size the buffer as the dimension of the file
        file_buffer.resize(file_size);

        // Position the pointer to the beginning of the file
        file.seekg(0);

        // Read the entire buffer
        file.read(file_buffer.data(), file_size);

        VkShaderModuleCreateInfo create_info{};

        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = file_buffer.size();
        create_info.pCode = reinterpret_cast<const uint32_t *>(file_buffer.data());

        if (vkCreateShaderModule(l_device->getDevice(), &create_info, nullptr, &shader) != VK_SUCCESS)
        {
            throw std::runtime_error("[Shader] Impossible to create the shader");
        }

        file.close();
    }

    Shader::~Shader()
    {
        vkDestroyShaderModule(l_device->getDevice(), shader, nullptr);
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