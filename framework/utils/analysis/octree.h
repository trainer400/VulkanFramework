#pragma once

#include <vulkan/vulkan.h>
#include <libs/glm/glm.hpp>

#include <memory>
#include <array>
#include <vector>

namespace framework
{
    class Octree
    {
    public:
        struct OctreeNode
        {
            bool empty = true;

            // Bounding box descriptor
            glm::vec3 origin;
            glm::vec3 opposite_corner;

            // Linked list with unique ptr for memory safety
            std::array<std::unique_ptr<OctreeNode>, 8> nodes;
        };

        Octree(const std::vector<glm::vec3> &vertices, const std::vector<uint32_t> &indices);

    private:
        std::unique_ptr<OctreeNode> head;
    };
}