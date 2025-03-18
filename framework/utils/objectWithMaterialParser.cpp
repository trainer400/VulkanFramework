#include "objectWithMaterialParser.h"

#include <sstream>
#include <stdexcept>
#include <fstream>

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
    }
}