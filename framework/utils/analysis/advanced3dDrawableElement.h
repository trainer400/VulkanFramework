#pragma once

#include <core/drawableElement.h>

#include <libs/glm/glm.hpp>

#include <memory>
#include <array>

namespace framework
{
    /**
     * @class Wrapper around the DrawableElement, that adds a triangle struct in order
     * to facilitate math with the triangle mesh. The idea is to have from the outside a classical
     * drawable element (this is reason for the inheritance) and
     */
    class Advanced3dDrawableElement : public DrawableElement
    {
    public:
        Advanced3dDrawableElement(std::unique_ptr<DrawableElement> element);

        void setUpdated() { element->setUpdated(); }

        // Getters
        const std::vector<float> &getVertices() { return element->getVertices(); }
        const std::vector<uint32_t> &getIndices() { return element->getIndices(); }
        const std::vector<VertexAttributes::DrawableAttribute> &getVertexAttributes() { return element->getVertexAttributes(); }
        bool isUpdated() { return element->isUpdated(); }
        bool isTransparent() { return element->isTransparent(); }

        void update() {}

    private:
        // Actual drawable element used to make this element drawable
        std::unique_ptr<DrawableElement> element;

        std::vector<glm::vec3> vertices_3d;
    };
}