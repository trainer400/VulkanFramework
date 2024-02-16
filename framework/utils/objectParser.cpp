#include "objectParser.h"

#include <fstream>
#include <stdexcept>
#include <sstream>

#define TINYOBJLOADER_IMPLEMENTATION
#include <libs/tiny_obj_loader.h>

using namespace std;

namespace framework
{
    ObjectParser::ObjectParser(const char *filename, bool hasTexture, bool hasNormals, bool rightHandedRef, float multiplication_factor)
    {
        if (filename == nullptr)
        {
            throw runtime_error("[ObjectParser] Null filename");
        }

        if (multiplication_factor <= 0)
        {
            throw runtime_error("[ObjectParser] Bad multiplication factor (<= 0)");
        }

        this->MULTIPLICATION_FACTOR = multiplication_factor;

        parse(filename, hasTexture, hasNormals, rightHandedRef);
    }

    void ObjectParser::parse(const char *filename, bool hasTexture, bool hasNormals, bool rightHandedRef)
    {
        // Instantiate all the tinyOBJ loader objects
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename))
        {
            throw std::runtime_error("[ObjectParser] Error from tiny-OBJ: " + warn + err);
        }

        vertexSize = 3 + (hasTexture ? 2 : 0) + (hasNormals ? 3 : 0);

        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                // Insert the vertex
                vertices.push_back((rightHandedRef ? -1 : 1) * attrib.vertices[3 * index.vertex_index + 0] * MULTIPLICATION_FACTOR);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 1] * MULTIPLICATION_FACTOR);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 2] * MULTIPLICATION_FACTOR);

                if (hasTexture)
                {
                    // Insert the texture coordinates
                    vertices.push_back(attrib.texcoords[2 * index.texcoord_index + 0]);
                    vertices.push_back(attrib.texcoords[2 * index.texcoord_index + 1]);
                }

                if (hasNormals)
                {
                    // Insert the normal coordinates
                    vertices.push_back((rightHandedRef ? -1 : 1) * attrib.normals[3 * index.normal_index + 0]);
                    vertices.push_back(attrib.normals[3 * index.normal_index + 1]);
                    vertices.push_back(attrib.normals[3 * index.normal_index + 2]);
                }

                // Insert the index
                indices.push_back((vertices.size() / vertexSize) - 1);

                // If just finished to insert a triangle, when it is a right handed reference, swap the indices
                if (rightHandedRef && indices.size() % 3 == 0)
                {
                    uint32_t temp = indices[indices.size() - 1];
                    indices[indices.size() - 1] = indices[indices.size() - 2];
                    indices[indices.size() - 2] = temp;
                }
            }
        }
    }
}