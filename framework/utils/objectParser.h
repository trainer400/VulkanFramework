#pragma once

#include <vector>
#include <stdint.h>
#include <string>

namespace framework
{
    struct ObjectParserConfiguration
    {
        bool hasTexture = true;
        bool hasNormals = true;
        bool rightHandedRef = true;
        bool addMedians = false;
        float multiplicationFactor = 1.0f;
    };

    class ObjectParser
    {
    public:
        ObjectParser(const char *filename, const ObjectParserConfiguration &config);

        // Getters
        inline const std::vector<float> &getVertices() { return vertices; }
        inline const std::vector<uint32_t> &getIndices() { return indices; }
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

        uint32_t vertexSize = 0;
    };
}