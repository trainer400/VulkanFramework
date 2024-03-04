#version 450

layout(binding = 0) uniform globalUniformBuffer
{
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 spawnPlaneDirection;
    float colorSpawnPlaneMagnitude;
} gubo;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormals;
layout (location = 3) in vec3 inMedian;
 
layout (location = 0) out vec2 outTexCoord;
layout (location = 1) out vec3 outMedian;
layout (location = 2) out vec4 outFragPos;

void main()
{
    gl_Position = gubo.projection * gubo.view * gubo.model * vec4(inPosition, 1.0f);
    outTexCoord = inTexCoord;
    outMedian = inMedian;
    outFragPos = gubo.model * vec4(inPosition, 1.0f);
}