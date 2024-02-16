#pragma once

#include <vector>
#include <stdint.h>
#include <string>

namespace framework
{
    class ObjectParser
    {
    public:
        ObjectParser(const char *filename, bool hasTexture, bool hasNormals, bool rightHandedRef, float multiplication_factor = 1.0f);

        // Getters
        inline const std::vector<float> &getVertices() { return vertices; }
        inline const std::vector<uint32_t> &getIndices() { return indices; }
        inline uint32_t getVertexSize() { return vertexSize; }

    private:
        /**
         * @brief Parses the object file
         */
        void parse(const char *filename, bool hasTexture, bool hasNormals, bool rightHandedRef);

        float MULTIPLICATION_FACTOR;

        std::vector<float> vertices;
        std::vector<uint32_t> indices;

        uint32_t vertexSize = 0;
    };
}