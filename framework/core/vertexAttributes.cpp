#include "vertexAttributes.h"

namespace framework
{
    bool VertexAttributes::operator==(const std::vector<DrawableAttribute> &other)
    {
        // Check if the sizes differ
        if(other.size() != this->attributes.size())
            return false;
        
        // In case some format is different return false
        for (size_t i = 0; i < other.size(); i++)
        {
            if(other.at(i) != this->attributes.at(i))
                return false;
        }

        return true;
    }
}