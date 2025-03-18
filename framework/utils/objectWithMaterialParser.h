#pragma once

#include <vector>
#include <stdint.h>
#include <string>

#include "objectParser.h"

namespace framework
{
    class ObjectWithMaterialParser
    {
    public:
        ObjectWithMaterialParser(const char *filename, const ObjectParserConfiguration &config);

        // Getters
        inline const std::vector<float> &getVertices() { return vertices; }
        inline const std::vector<uint32_t> &getIndices() { return indices; }
        inline const std::vector<std::string> &getDiffuseTexturePaths() { return diffuseTexturePaths; }
        inline uint32_t getVertexSize() { return vertexSize; }

    private:
        /**
         * @brief Parses the object file
         */
        void parse(const char *filename);

        // Configuration
        ObjectParserConfiguration config;

        std::vector<float> vertices;
        std::vector<uint32_t> indices;
        std::vector<std::string> diffuseTexturePaths;

        uint32_t vertexSize = 0;
    };
}