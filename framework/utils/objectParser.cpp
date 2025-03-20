#include "objectParser.h"

#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include <libs/tiny_obj_loader.h>

using namespace std;

namespace framework
{
    ObjectParser::ObjectParser(const char *filename, const ObjectParserConfiguration &config) : config(config)
    {
        if (filename == nullptr)
        {
            throw runtime_error("[ObjectParser] Null filename");
        }

        if (config.multiplicationFactor <= 0)
        {
            throw runtime_error("[ObjectParser] Bad multiplication factor (<= 0)");
        }

        parse(filename);
    }

    void ObjectParser::parse(const char *filename)
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

        vertexSize = 3 + (config.hasTexture ? 2 : 0) + (config.hasNormals ? 3 : 0) + (config.addMedians ? 3 : 0);

        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                // Insert the vertex
                vertices.push_back((config.rightHandedRef ? -1 : 1) * attrib.vertices[3 * index.vertex_index + 0] * config.multiplicationFactor);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 1] * config.multiplicationFactor);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 2] * config.multiplicationFactor);

                if (config.hasTexture)
                {
                    // Insert the texture coordinates
                    vertices.push_back(attrib.texcoords[2 * index.texcoord_index + 0]);
                    vertices.push_back((config.invertTexture ? -1 : 1) * attrib.texcoords[2 * index.texcoord_index + 1]);
                }

                if (config.hasNormals)
                {
                    // Insert the normal coordinates
                    vertices.push_back((config.rightHandedRef ? -1 : 1) * attrib.normals[3 * index.normal_index + 0]);
                    vertices.push_back(attrib.normals[3 * index.normal_index + 1]);
                    vertices.push_back(attrib.normals[3 * index.normal_index + 2]);
                }

                if (config.addMedians)
                {
                    // Insert point with 1 on the axis of this index % 3
                    std::vector<float> med;
                    med.insert(med.end(), {0, 0, 0});

                    // Total length is 1 and the position depends on the vertex in the triangle
                    med[indices.size() % 3] = 1.0f;

                    // Insert the median inside the vertices
                    vertices.push_back(med[0]);
                    vertices.push_back(med[1]);
                    vertices.push_back(med[2]);
                }

                // Insert the index
                indices.push_back((vertices.size() / vertexSize) - 1);

                // If just finished to insert a triangle, when it is a right handed reference, swap the indices
                if (config.rightHandedRef && indices.size() % 3 == 0)
                {
                    uint32_t temp = indices[indices.size() - 1];
                    indices[indices.size() - 1] = indices[indices.size() - 2];
                    indices[indices.size() - 2] = temp;
                }
            }
        }
    }
}