#pragma once

#include <vector>
#include <stdint.h>
#include <string>
#include <memory>

#include <core/drawableElement.h>

namespace framework
{
    struct ObjectParserConfiguration
    {
        bool hasTexture = true;
        bool hasNormals = true;
        bool rightHandedRef = true;
        bool addMedians = false;
        bool invertTexture = false;
        float multiplicationFactor = 1.0f;
    };

    /**
     * @brief The method parses an object file with the passed configuration.
     * The user can decide to include textures/normals, to scale the model and 
     * eventually to add medians to it. The method is also tolerant to different
     * texture mapping formats, having a flag named "invertTexture". 
     * 
     * @param tex_paths is a user vector that is modified by the method to include the texture names in the correct order.
     * @warning The texture ordering is the same which is then used by every vertex inside memory, so it is important
     * to keep the initial object ordering.
     */
    std::vector<std::shared_ptr<DefaultDrawableElement>> parseObjFile(const char *filename, const ObjectParserConfiguration &config, std::vector<std::string>& tex_paths);
}