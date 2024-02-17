#include <stdio.h>
#include <iostream>
#include <framework/core/vulkan.h>
#include <framework/window/window.h>

using namespace std;
using namespace framework;

shared_ptr<Vulkan> vulkan;
shared_ptr<Window> window;
shared_ptr<LogicalDevice> lDevice;
shared_ptr<CommandPool> commandPool;
unique_ptr<CommandBuffer> commandBuffer;
shared_ptr<Pipeline> cubePipeline;
unique_ptr<SwapChain> swapChain;
unique_ptr<RenderPass> renderPass;
unique_ptr<FrameBufferCollection> frameBufferCollection;

class Cube : public DrawableElement
{
public:
    Cube()
    {
        // clang-format off
        
        vertices.insert(vertices.end(), {-1, -1, 0, 0, 1, 0});
        vertices.insert(vertices.end(), {1, -1, 0, 1, 0, 0});
        vertices.insert(vertices.end(), {-1, 1, 0, 0, 0, 1});
        vertices.insert(vertices.end(), {1, 1, 0, 0, 1, 0});

        indices.insert(indices.end(), {0, 1, 2, 1, 2, 3});

        // clang-format on
    }

    void update() override
    {
    }
};

void createGraphicsObjects()
{
    shared_ptr<Shader> vertex = make_shared<Shader>(lDevice, "examples/cube/shaders/vert.spv", ShaderType::VERTEX);
    shared_ptr<Shader> fragment = make_shared<Shader>(lDevice, "examples/cube/shaders/frag.spv", ShaderType::FRAGMENT);

    // Read the shaders
    vector<shared_ptr<Shader>> shaders;
    shaders.push_back(vertex);
    shaders.push_back(fragment);

    // Create the drawable collection
    unique_ptr<DrawableCollection> cubeCollection = make_unique<DrawableCollection>(lDevice, nullptr, commandPool->getCommandPool(), shaders);
    cubeCollection->addElement(make_shared<Cube>());
    cubeCollection->addAttribute(DrawableAttribute::F3);
    cubeCollection->addAttribute(DrawableAttribute::F3);
    cubeCollection->allocate();

    // Create the pipeline
    PipelineConfiguration config{};
    config.cullMode = VK_CULL_MODE_NONE;
    cubePipeline = make_shared<Pipeline>(lDevice, move(cubeCollection), renderPass->getDepthTestType(), renderPass->getRenderPass(), config);

    // Add the pipeline to the renderer
    vulkan->addPipeline(cubePipeline);
}

void onUpdate()
{
    vulkan->draw({0.0f, 0.0f, 0.0f, 1.0f});
}

void onClose()
{
    lDevice->waitIdle();
}

void onUpdateSize()
{
    vulkan->manageResize(window);
}

int main()
{
    // Requested extensions
    std::vector<const char *> extensions;

    // Create window and environment
    window = make_shared<Window>(1280, 720, "Cube", true);
    vulkan = make_shared<Vulkan>("Cube", "No Engine", extensions, true);
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

    createGraphicsObjects();

    // Select the graphical objects
    vulkan->selectSurface(move(surface));
    vulkan->selectLogicalDevice(lDevice);
    vulkan->selectSwapChain(move(swapChain));
    vulkan->selectRenderPass(move(renderPass));
    vulkan->selectFrameBufferCollection(move(frameBufferCollection));
    vulkan->selectCommandBuffer(move(commandBuffer));
    vulkan->createSyncObjects();

    // Create imgui
    vulkan->setupImGui(window->getWindow(), [&]() {});

    // Run the window
    window->run([=]()
                { onUpdate(); },
                [=]()
                { onUpdateSize(); },
                [=]()
                { onClose(); });

    return 0;
}