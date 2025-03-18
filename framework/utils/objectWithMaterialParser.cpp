#include "objectWithMaterialParser.h"

#include <sstream>
#include <stdexcept>
#include <fstream>
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include <libs/tiny_obj_loader.h>

using namespace std;

namespace framework
{
    ObjectWithMaterialParser::ObjectWithMaterialParser(const char *filename, const ObjectParserConfiguration &config) : config(config)
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

    void ObjectWithMaterialParser::parse(const char *filename)
    {
        // Instantiate all the tinyOBJ loader objects
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        // Set the material path as the filename
        std::string mtl_path{filename};

        // Extract the material path as the same folder of the .obj file
        {
            // Add ./ to filename
            mtl_path = "./" + mtl_path;

            // Look for the last /
            int index;
            for (index = mtl_path.size() - 1; index >= 0 && mtl_path[index] != '/'; index--)
                ;
            mtl_path = mtl_path.substr(0, index + 1);
        }

        // Load the object
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, mtl_path.c_str()))
        {
            throw std::runtime_error("[ObjectParser] Error from tiny-OBJ: " + warn + err);
        }
    }
}