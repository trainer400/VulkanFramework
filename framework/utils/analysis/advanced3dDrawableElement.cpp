#include "advanced3dDrawableElement.h"

namespace framework
{
    // TODO: The method assumes that the 3D coordinates are the first 3 floats inside the vertices array
    Advanced3dDrawableElement::Advanced3dDrawableElement(std::unique_ptr<DrawableElement> element)
    {
        const auto &vertices = element->getVertices();
        const auto &indices = element->getIndices();
        const auto &vertex_attributes = element->getVertexAttributes();

        // Read the number of "floats" (or equivalent) per vertex
        // TODO: incorporate attributes object inside the drawable element
        VertexAttributes attributes{vertex_attributes};
        unsigned long floats_per_vertex = attributes.byteSize() / sizeof(float);
        uint32_t number_of_vertices = vertices.size() / floats_per_vertex;

        // Reserve space for the vector causing one only big allocation
        vertices_3d.reserve(number_of_vertices);

        // Add the vertices inside the struct (XYZ convention)
        for (uint32_t i = 0; i < number_of_vertices; i++)
        {
            uint32_t x_index = i * floats_per_vertex;

            float x = vertices.at(x_index);
            float y = vertices.at(x_index + 1);
            float z = vertices.at(x_index + 2);

            vertices_3d.push_back(glm::vec3(x, y, z));
        }
    }
}