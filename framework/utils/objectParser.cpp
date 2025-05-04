#include "objectParser.h"

#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <sstream>
#include <algorithm>

#define TINYOBJLOADER_IMPLEMENTATION
#include <libs/tiny_obj_loader.h>

using namespace std;

namespace framework
{
    /**
     * @brief Returns the folder path into which it is stored the passed file
     * @warning TODO: IT WORKS ONLY ON LINUX PATHS, MAKE IT WORK EVERYWHERE
     */
    std::string getFolderPath(const std::string &filename)
    {
        // Extract folder path as the same folder of the specified file
        std::string folder_path{filename};

        // Add ./ to filename
        folder_path = "./" + folder_path;

        // Look for the last /
        int index;
        for (index = folder_path.size() - 1; index >= 0 && folder_path[index] != '/'; index--)
            ;
        folder_path = folder_path.substr(0, index + 1);

        return folder_path;
    }

    /**
     * @brief Given a set of tinyobj materials, the method returns a ordered set of
     * textures filepaths which can be then loaded into GPU memory.
     * @warning The texture index to which every vertex references to is related to
     * the output order of this vector.
     */
    std::vector<std::string> getTexturePaths(const std::string &mtl_file_folder, const std::vector<tinyobj::material_t> &materials)
    {
        std::vector<std::string> result;

        // Populate the texture paths (TODO use different texture types other than diffuse lights)
        for (const auto &mat : materials)
        {
            std::string path = "";
            if (mat.diffuse_texname != "")
                path = mtl_file_folder + mat.diffuse_texname;
            else if (mat.alpha_texname != "")
                path = mtl_file_folder + mat.alpha_texname;

            // Replace \ path with /
            std::replace(path.begin(), path.end(), '\\', '/');
            result.push_back(path);
        }

        return result;
    }

    /**
     * @brief Given the tinyobj shape, the method parses its vertices/indices producing
     * a final drawable element which can be then rendered by the framework.
     */
    std::shared_ptr<DefaultDrawableElement> getParsedDrawableElement(uint32_t vertex_size,
                                                                     const tinyobj::shape_t &shape,
                                                                     const tinyobj::attrib_t &attrib,
                                                                     const ObjectParserConfiguration &config)
    {
        std::vector<float> vertices;
        std::vector<uint32_t> indices;

        for (size_t i = 0; i < shape.mesh.indices.size(); i++)
        {
            const tinyobj::index_t &index = shape.mesh.indices[i];

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

            // Insert the material index inside the vertex (/3 because it is the same for every vertex in the same triangle)
            vertices.push_back(shape.mesh.material_ids[i / 3]);

            // Insert the index
            indices.push_back((vertices.size() / vertex_size) - 1);

            // If just finished to insert a triangle, when it is a right handed reference, swap the indices
            if (config.rightHandedRef && indices.size() % 3 == 0)
            {
                uint32_t temp = indices[indices.size() - 1];
                indices[indices.size() - 1] = indices[indices.size() - 2];
                indices[indices.size() - 2] = temp;
            }
        }

        // Create the result drawable object
        return std::make_shared<DefaultDrawableElement>(vertices, indices);
    }

    std::vector<std::shared_ptr<DefaultDrawableElement>> parseObjFile(const char *filename, const ObjectParserConfiguration &config, std::vector<std::string>& tex_paths)
    {
        if (filename == nullptr)
            throw runtime_error("[ObjectParser] Null filename");

        if (config.multiplicationFactor <= 0)
            throw runtime_error("[ObjectParser] Bad multiplication factor (<= 0)");

        // Instantiate all the tinyOBJ loader objects
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        // Get the material directory
        std::string mtl_file_folder = getFolderPath(filename);

        // Instantiate the result variables
        std::vector<std::shared_ptr<DefaultDrawableElement>> result;

        // Load the object
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, mtl_file_folder.c_str()))
            throw std::runtime_error("[ObjectParser] Error from tiny-OBJ: " + warn + err);

        // The last +1 is for the material index (TODO use uint for material index)
        uint32_t vertex_size = 3 + (config.hasTexture ? 2 : 0) + (config.hasNormals ? 3 : 0) + (config.addMedians ? 3 : 0) + 1;

        // Get all the texture paths
        tex_paths = getTexturePaths(mtl_file_folder, materials);

        // Parse all the shapes
        for (const auto &shape : shapes)
            result.push_back(getParsedDrawableElement(vertex_size, shape, attrib, config));

        return result;
    }
}