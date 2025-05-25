#include "vertexAttributes.h"

namespace framework
{
    bool VertexAttributes::operator==(const std::vector<DrawableAttribute> &other)
    {
        // Check if the sizes differ
        if (other.size() != this->attributes.size())
            return false;

        // In case some format is different return false
        for (size_t i = 0; i < other.size(); i++)
        {
            if (other.at(i) != this->attributes.at(i))
                return false;
        }

        return true;
    }

    unsigned long VertexAttributes::byteSize(const DrawableAttribute &a)
    {
        switch (a)
        {
        case VertexAttributes::DrawableAttribute::I1:
            return 1 * sizeof(uint32_t);
        case VertexAttributes::DrawableAttribute::F1:
            return 1 * sizeof(float);
        case VertexAttributes::DrawableAttribute::F2:
            return 2 * sizeof(float);
        case VertexAttributes::DrawableAttribute::F3:
            return 3 * sizeof(float);
        case VertexAttributes::DrawableAttribute::F4:
            return 4 * sizeof(float);
        default:
            return 0;
        }
    }

    unsigned long VertexAttributes::byteSize()
    {
        unsigned long size_of_struct = 0;

        for (int i = 0; i < attributes.size(); i++)
            size_of_struct += byteSize(attributes[i]);

        return size_of_struct;
    }

}