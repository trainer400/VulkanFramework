#include "drawableElement.h"

namespace framework
{
    Default3DDrawableElement::Default3DDrawableElement(const std::vector<float> &vertices, const std::vector<VertexAttributes::DrawableAttribute> &vertex_attributes,
                                                       const std::vector<uint32_t> &indices, bool transparent) : DefaultDrawableElement(vertices, vertex_attributes, indices, transparent)
    {
    }
}