#pragma once

#include <vector>
#include <stdint.h>

#include <core/vertexAttributes.h>
namespace framework
{
    class DrawableElement
    {
    public:
        /**
         * @brief Update function that triggers the internal drawable object update
         */
        virtual void update() = 0;

        /**
         * @brief Sets the updated flag to false, indicating that the vertices and the indices
         * have been copied into the GPU memory
         */
        void setUpdated() { updated = false; }

        // Getters
        const std::vector<float> &getVertices() { return vertices; }
        const std::vector<uint32_t> &getIndices() { return indices; }
        const std::vector<VertexAttributes::DrawableAttribute> &getVertexAttributes() { return vertex_attributes; }
        bool isUpdated() { return updated; }

    protected:
        std::vector<float> vertices;
        std::vector<uint32_t> indices;
        std::vector<VertexAttributes::DrawableAttribute> vertex_attributes;

        bool updated = true;
    };

    class DefaultDrawableElement : public DrawableElement
    {
    public:
        DefaultDrawableElement(const std::vector<float> &vertices, const std::vector<VertexAttributes::DrawableAttribute> &vertex_attributes,
                               const std::vector<uint32_t> &indices, bool transparent)
        {
            this->vertices = vertices;
            this->indices = indices;
            this->vertex_attributes = vertex_attributes;
            this->is_transparent = transparent;
        }

        void update() {}

        bool isTransparent() { return is_transparent; }

    private:
        bool is_transparent = false;
    };
}