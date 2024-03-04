#version 450

layout(binding = 0) uniform globalUniformBuffer
{
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 spawnPlaneDirection;
    float colorSpawnPlaneMagnitude;
} gubo;

layout (location = 0) in vec2 inTexCoord;
layout (location = 1) in vec3 inMedian;
layout (location = 2) in vec4 inFragPos;

layout (location = 0) out vec4 outColor;

layout (binding = 1) uniform sampler2D tex;

#define BOUNDARIES 50

void main()
{
    // By default no color
    outColor = vec4(0, 0, 0, 0);

    // Scale the plane vector
    vec3 normalizedPlaneDirection = normalize(gubo.spawnPlaneDirection);
    float pos = dot(inFragPos.xyz, normalizedPlaneDirection);

    // Change the reference plane with a sin function to avoid the straight plane cut
    float magnitude = gubo.colorSpawnPlaneMagnitude + (BOUNDARIES / 4) * sin(length(inFragPos.xyz - normalizedPlaneDirection * gubo.colorSpawnPlaneMagnitude) / (BOUNDARIES / 4));

    // Assign the color if the plane has passed the object
    if(pos < magnitude)
    {
        // Assign the color
        outColor = texture(tex, inTexCoord);
    }
    
    // Assign the wireframe if the object is the the middle of the plane 
    if(pos <= magnitude + BOUNDARIES && pos > magnitude - BOUNDARIES)
    {
        if(inMedian.x < 0.008 || inMedian.y < 0.008 || inMedian.z < 0.008 || abs(pos - magnitude - BOUNDARIES) < 0.5)
        {
            outColor = vec4(1.0, 1.0, 1.0, 1.0);
        }
    }
}