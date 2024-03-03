#version 450

layout(binding = 0) uniform globalUniformBuffer
{
    mat4 model;
    mat4 view;
    mat4 projection;
} gubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormals;

layout(location = 0) out vec2 outTexCoord;

void main()
{
    gl_Position = gubo.projection * gubo.view * gubo.model * vec4(inPosition, 1.0f);
    outTexCoord = inTexCoord;
}