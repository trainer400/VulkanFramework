#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace framework
{
    class VertexAttributes
    {
    public:
        /**
         * uint32: VK_FORMAT_R32_UINT
         * float: VK_FORMAT_R32_SFLOAT
         * vec2: VK_FORMAT_R32G32_SFLOAT
         * vec3: VK_FORMAT_R32G32B32_SFLOAT
         * vec4: VK_FORMAT_R32G32B32A32_SFLOAT
         */
        enum DrawableAttribute : uint32_t
        {
            F1 = VK_FORMAT_R32_SFLOAT,
            F2 = VK_FORMAT_R32G32_SFLOAT,
            F3 = VK_FORMAT_R32G32B32_SFLOAT,
            F4 = VK_FORMAT_R32G32B32A32_SFLOAT,
            I1 = VK_FORMAT_R32_UINT
        };

        VertexAttributes(const std::vector<DrawableAttribute> &vertex_attributes) : attributes(vertex_attributes) {}
        VertexAttributes(const VertexAttributes &other) { this->attributes = other.attributes; }
        VertexAttributes(const VertexAttributes &&other) = delete;

        const std::vector<DrawableAttribute> &getVertexAttributes() { return attributes; }

        bool operator==(const std::vector<DrawableAttribute> &other);
        bool operator==(const VertexAttributes &other) { return operator==(other.attributes); }

        // Return the size in bytes of the entire struct
        unsigned long byteSize();
        unsigned long byteSize(const DrawableAttribute &a);
        size_t size() { return attributes.size(); }

    private:
        std::vector<DrawableAttribute> attributes;
    };
}