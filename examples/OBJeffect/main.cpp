#include <iostream>
#include <libs/glm/glm.hpp>
#include <libs/glm/gtc/matrix_transform.hpp>
#include <framework/core/vulkan.h>
#include <framework/core/uniformBuffer.h>
#include <framework/core/texture.h>
#include <framework/utils/defaultRenderer.h>
#include <framework/utils/objectParser.h>
#include <framework/utils/FPSCamera.h>

using namespace std;
using namespace framework;

shared_ptr<Vulkan> vulkan;
shared_ptr<Window> window;
shared_ptr<LogicalDevice> lDevice;
shared_ptr<CommandPool> commandPool;
unique_ptr<CommandBuffer> commandBuffer;
shared_ptr<Pipeline> objPipeline;
unique_ptr<SwapChain> swapChain;
unique_ptr<RenderPass> renderPass;
unique_ptr<FrameBufferCollection> frameBufferCollection;
unique_ptr<DefaultRenderer> renderer;
FPSCamera camera{100, 45, 0.1f, 10000.0f, {}};
bool mouseFix = true; // Sets the mouse position at the center

// Test variables
glm::vec3 planeDirection;
float colorMagnitude;

struct GlobalUniformBuffer
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec3 spawnPlaneDirection;
    alignas(4) float colorSpawnPlaneMagnitude;
};

// Uniform buffer
shared_ptr<UniformBuffer<GlobalUniformBuffer>> gubo;
shared_ptr<Texture> texture;

void createGraphicsObjects()
{
    shared_ptr<Shader> vertex = make_shared<Shader>(lDevice, "examples/OBJeffect/shaders/vert.spv", ShaderType::VERTEX);
    shared_ptr<Shader> fragment = make_shared<Shader>(lDevice, "examples/OBJeffect/shaders/frag.spv", ShaderType::FRAGMENT);

    // Read the shaders
    vector<shared_ptr<Shader>> shaders;
    shaders.push_back(vertex);
    shaders.push_back(fragment);

    // Create the uniform buffer
    UniformBufferConfiguration guboConfig;
    guboConfig.bindingIndex = 0;
    guboConfig.stageFlags = VK_SHADER_STAGE_ALL;
    gubo = make_shared<UniformBuffer<GlobalUniformBuffer>>(lDevice, guboConfig);

    // Parse the obj file
    std::vector<std::string> textures;
    ObjectParserConfiguration parser_config;
    parser_config.addMedians = true;
    std::vector<std::shared_ptr<DefaultDrawableElement>> drawable_elements = parseObjFile("examples/OBJeffect/models/Rock_5.obj", parser_config, textures);

    // Create the texture
    texture = make_shared<Texture>(lDevice, commandPool->getCommandPool(), textures[0].c_str(), 1);

    // Create the descriptor set
    std::vector<shared_ptr<DescriptorElement>> elements;
    elements.push_back(gubo);
    elements.push_back(texture);

    unique_ptr<DescriptorSet> descriptor = make_unique<DescriptorSet>(lDevice, elements);

    // Create the drawable collection
    unique_ptr<DrawableCollection> cubeCollection = make_unique<DrawableCollection>(lDevice, move(descriptor), commandPool->getCommandPool(), shaders);
    
    // Add the drawable elements
    for (auto &e : drawable_elements)
        cubeCollection->addElement(e);
    cubeCollection->allocate();

    // Create the pipeline
    PipelineConfiguration config{};
    config.cull_mode = VK_CULL_MODE_NONE;
    objPipeline = make_shared<Pipeline>(lDevice, move(cubeCollection), renderPass->getDepthTestType(), renderPass->getRenderPass(), config);

    // Add the pipeline to the renderer
    renderer->addPipeline(objPipeline);
}

void updateUniformBuffer()
{
    GlobalUniformBuffer buf{};

    // Set the plane values
    buf.spawnPlaneDirection = planeDirection;
    buf.colorSpawnPlaneMagnitude = colorMagnitude;

    // Set the data to transfer
    buf.model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.f), glm::vec3(0, 1.0f, 0)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(90.f), glm::vec3(1.0, 0, 0));
    buf.view = camera.getLookAtMatrix();
    buf.projection = camera.getPerspectiveMatrix(window->getWidth(), window->getHeight());

    // Transfer the data
    gubo->setData(buf);
}

void onUpdate()
{
    updateUniformBuffer();

    // Draw the frame
    renderer->draw({0.0f, 0.0f, 0.0f, 1.0f});

    // Set the mouse position
    if (mouseFix)
    {
        // Update the camera
        camera.updatePosition();
        glfwSetCursorPos(window->getWindow(), window->getWidth() / 2, window->getHeight() / 2);
        glfwSetInputMode(window->getWindow(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else
    {
        glfwSetInputMode(window->getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void onClose()
{
    lDevice->waitIdle();
}

void onUpdateSize()
{
    renderer->manageResize(window);
}

void setupGui()
{
    ImGui::Begin("Prova");

    // Create slider for spawn plane and magnitudes
    ImGui::DragFloat3("Plane direction", &planeDirection.x, 0.001f, -1.0f, 1.0f, "%.3f");
    ImGui::DragFloat("Color magnitude", &colorMagnitude, 10.0f, -10000.0f, 10000.0f, "%.1f");

    ImGui::End();
}

int main()
{
    // Requested extensions
    std::vector<const char *> extensions;

    // Create window and environment
    window = make_shared<Window>(1280, 720, "OBJeffect", true);
    vulkan = make_shared<Vulkan>("OBJeffect", "No Engine", extensions, true);
    unique_ptr<WindowSurface> surface = make_unique<WindowSurface>(vulkan->getInstance(), window->getWindow());

    // Create devices
    unique_ptr<PhysicalDevice> pDevice = make_unique<PhysicalDevice>(vulkan->getInstance(), surface->getSurface(), 0);
    lDevice = make_shared<LogicalDevice>(move(pDevice), surface->getSurface());

    // Create command buffer and pool
    commandPool = make_shared<CommandPool>(lDevice, surface->getSurface());
    commandBuffer = make_unique<CommandBuffer>(lDevice, commandPool->getCommandPool());

    // Create frame buffer collection
    swapChain = make_unique<SwapChain>(lDevice, window, surface->getSurface(), SwapChainConfiguration{});
    renderPass = make_unique<RenderPass>(lDevice, swapChain->getExtent(), swapChain->getFormat(), DepthTestType::DEPTH_32);
    frameBufferCollection = make_unique<FrameBufferCollection>(lDevice, swapChain->getImageViews(), swapChain->getExtent(),
                                                               renderPass->getDepthTestType(), renderPass->getDepthImageView(), renderPass->getRenderPass());

    // Create the renderer object
    renderer = make_unique<DefaultRenderer>();

    // Create the object to render
    createGraphicsObjects();

    // Select the graphical objects
    renderer->selectInstance(vulkan);
    renderer->selectSurface(move(surface));
    renderer->selectLogicalDevice(lDevice);
    renderer->selectSwapChain(move(swapChain));
    renderer->selectRenderPass(move(renderPass));
    renderer->selectFrameBufferCollection(move(frameBufferCollection));
    renderer->selectCommandBuffer(move(commandBuffer));

    // Create imgui
    renderer->setupImGui(window->getWindow(), [&]()
                         { setupGui(); });

    // Setup camera
    camera.setPosition({0, 2.0f, -10.0f});
    camera.registerCallbacks(window);

    // Add a callback for ESC
    window->addKeyCallback(GLFW_KEY_ESCAPE, [&](int key, int action)
                           { if(action == GLFW_PRESS){mouseFix = !mouseFix;} });

    // Setup test variables
    planeDirection = {-1, 0, 0};
    colorMagnitude = 1.f;

    // Run the window
    window->run([=]()
                { onUpdate(); },
                [=]()
                { onUpdateSize(); },
                [=]()
                { onClose(); });

    return 0;
}