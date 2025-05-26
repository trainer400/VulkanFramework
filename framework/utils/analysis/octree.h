#pragma once

#include <core/drawableElement.h>
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
            bool leaf = false;

            // Bounding box descriptor
            glm::vec3 origin;
            glm::vec3 opposite_corner;

            // Linked list with unique ptr for memory safety
            std::array<std::unique_ptr<OctreeNode>, 8> nodes;

            // Collection of indices that the node references to. It is empty if the node is not a leaf.
            std::vector<uint32_t> triangle_indices;
        };

        Octree(std::shared_ptr<std::vector<glm::vec3>> vertices, const std::vector<uint32_t> &indices);

    private:
        std::unique_ptr<OctreeNode> head;
    };
}