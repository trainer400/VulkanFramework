#include "drawableElement.h"

#include <iostream>

namespace framework
{
    // TODO: The method assumes that the 3D coordinates are the first 3 floats inside the vertices array
    Default3DDrawableElement::Default3DDrawableElement(const std::vector<float> &vertices, const std::vector<VertexAttributes::DrawableAttribute> &vertex_attributes,
                                                       const std::vector<uint32_t> &indices, bool transparent) : DefaultDrawableElement(vertices, vertex_attributes, indices, transparent)
    {
        if (vertices.size() == 0)
            throw std::runtime_error("[Default3dDrawableElement] Empty vertices vector");
        if (indices.size() == 0)
            throw std::runtime_error("[Default3dDrawableElement] Empty indices vector");
        if (vertex_attributes.size() == 0)
            throw std::runtime_error("[Default3dDrawableElement] Empty attributes vector");
        // Check that the first vertex attribute is actually a vec3
        if (vertex_attributes.at(0) != VertexAttributes::DrawableAttribute::F3)
            throw std::runtime_error("[Default3dDrawableElement] The first vertex attribute is not a 3D position, is the object 3D?");

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