#version 450

layout (location = 0) in vec2 inTexCoord;
layout (location = 1) in vec3 inMedian;
layout (location = 2) in vec4 inFragPos;
layout (location = 3) in vec3 inSpawnPlaneDirection;
layout (location = 4) in float inColorSpawnPlaneMagnitude;
layout (location = 5) in float inWireFrameSpawnPlaneMagnitude;

layout (location = 0) out vec4 outColor;

layout (binding = 1) uniform sampler2D tex;

void main()
{
    outColor = texture(tex, inTexCoord);

    if(inMedian.x < 0.008 || inMedian.y < 0.008 || inMedian.z < 0.008)
    {
        outColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
}