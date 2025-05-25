#include <stdio.h>
#include <iostream>
#include <chrono>
#include <libs/glm/glm.hpp>
#include <libs/glm/gtc/matrix_transform.hpp>
#include <framework/core/vulkan.h>
#include <framework/window/window.h>
#include <framework/utils/camera.h>
#include <framework/core/uniformBuffer.h>
#include <framework/utils/defaultRenderer.h>

using namespace std;
using namespace framework;

shared_ptr<Vulkan> vulkan;
shared_ptr<Window> window;
shared_ptr<LogicalDevice> l_device;
shared_ptr<CommandPool> command_pool;
unique_ptr<CommandBuffer> command_buffer;
shared_ptr<Pipeline> cube_pipeline;
unique_ptr<SwapChain> swap_chain;
unique_ptr<RenderPass> render_pass;
unique_ptr<FrameBufferCollection> frame_buffer_collection;
unique_ptr<DefaultRenderer> renderer;
Camera camera{45, 0.1f, 100.0f};

struct GlobalUniformBuffer
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
};

// Uniform buffer
shared_ptr<UniformBuffer<GlobalUniformBuffer>> gubo;

class Cube : public DrawableElement
{
public:
    Cube()
    {
        // clang-format off
        
        // Back
        vertices.insert(vertices.end(), {-1.0, -1.0, -1.0, 1.0f, 0.0f, 0.0f});
        vertices.insert(vertices.end(), {1.0, -1.0, -1.0, 1.0f, 0.0f, 0.0f});
        vertices.insert(vertices.end(), {-1.0, 1.0, -1.0, 1.0f, 0.0f, 0.0f});
        vertices.insert(vertices.end(), {1.0, 1.0, -1.0, 1.0f, 0.0f, 0.0f});
        indices.push_back(2); indices.push_back(1); indices.push_back(0); // First triangle
        indices.push_back(1); indices.push_back(2); indices.push_back(3); // Second triangle

        // Left
        vertices.insert(vertices.end(), {-1.0, -1.0, -1.0, 0.0f, 1.0f, 0.0f});
        vertices.insert(vertices.end(), {-1.0, -1.0, 1.0, 0.0f, 1.0f, 0.0f});
        vertices.insert(vertices.end(), {-1.0, 1.0, -1.0, 0.0f, 1.0f, 0.0f});
        vertices.insert(vertices.end(), {-1.0, 1.0, 1.0, 0.0f, 1.0f, 0.0f});
        indices.push_back(0 + 4); indices.push_back(1 + 4); indices.push_back(2 + 4); // First triangle
        indices.push_back(3 + 4); indices.push_back(2 + 4); indices.push_back(1 + 4); // Second triangle

        // Right
        vertices.insert(vertices.end(), {1.0, -1.0, -1.0, 0.0f, 1.0f, 0.0f});
        vertices.insert(vertices.end(), {1.0, -1.0, 1.0, 0.0f, 1.0f, 0.0f});
        vertices.insert(vertices.end(), {1.0, 1.0, -1.0, 0.0f, 1.0f, 0.0f});
        vertices.insert(vertices.end(), {1.0, 1.0, 1.0, 0.0f, 1.0f, 0.0f});
        indices.push_back(2 + 8); indices.push_back(1 + 8); indices.push_back(0 + 8); // First triangle
        indices.push_back(1 + 8); indices.push_back(2 + 8); indices.push_back(3 + 8); // Second triangle

        // Top
        vertices.insert(vertices.end(), {-1.0, 1.0, -1.0, 0.0f, 0.0f, 1.0f});
        vertices.insert(vertices.end(), {1.0, 1.0, -1.0, 0.0f, 0.0f, 1.0f});
        vertices.insert(vertices.end(), {-1.0, 1.0, 1.0, 0.0f, 0.0f, 1.0f});
        vertices.insert(vertices.end(), {1.0, 1.0, 1.0, 0.0f, 0.0f, 1.0f});
        indices.push_back(2 + 12); indices.push_back(1 + 12); indices.push_back(0 + 12); // First triangle
        indices.push_back(1 + 12); indices.push_back(2 + 12); indices.push_back(3 + 12); // Second triangle

        // Bottom
        vertices.insert(vertices.end(), {-1.0, -1.0, -1.0, 0.0f, 0.0f, 1.0f});
        vertices.insert(vertices.end(), {1.0, -1.0, -1.0, 0.0f, 0.0f, 1.0f});
        vertices.insert(vertices.end(), {-1.0, -1.0, 1.0, 0.0f, 0.0f, 1.0f});
        vertices.insert(vertices.end(), {1.0, -1.0, 1.0, 0.0f, 0.0f, 1.0f});
        indices.push_back(0 + 16); indices.push_back(1 + 16); indices.push_back(2 + 16); // First triangle
        indices.push_back(3 + 16); indices.push_back(2 + 16); indices.push_back(1 + 16); // Second triangle
        
        // Front
        vertices.insert(vertices.end(), {-1.0, -1.0, 1.0, 1.0f, 0.0f, 0.0f});
        vertices.insert(vertices.end(), {1.0, -1.0, 1.0, 1.0f, 0.0f, 0.0f});
        vertices.insert(vertices.end(), {-1.0, 1.0, 1.0, 1.0f, 0.0f, 0.0f});
        vertices.insert(vertices.end(), {1.0, 1.0, 1.0, 1.0f, 0.0f, 0.0f});
        indices.push_back(0 + 20); indices.push_back(1 + 20); indices.push_back(2 + 20); // First triangle
        indices.push_back(3 + 20); indices.push_back(2 + 20); indices.push_back(1 + 20); // Second triangle

        // clang-format on

        this->vertex_attributes.push_back(VertexAttributes::DrawableAttribute::F3);
        this->vertex_attributes.push_back(VertexAttributes::DrawableAttribute::F3);
    }

    void update() override
    {
    }
};

void createGraphicsObjects()
{
    shared_ptr<Shader> vertex = make_shared<Shader>(l_device, "examples/cube/shaders/vert.spv", ShaderType::VERTEX);
    shared_ptr<Shader> fragment = make_shared<Shader>(l_device, "examples/cube/shaders/frag.spv", ShaderType::FRAGMENT);

    // Read the shaders
    vector<shared_ptr<Shader>> shaders;
    shaders.push_back(vertex);
    shaders.push_back(fragment);

    // Create the uniform buffer
    UniformBufferConfiguration gubo_config;
    gubo_config.binding_index = 0;
    gubo_config.stage_flags = VK_SHADER_STAGE_VERTEX_BIT;
    gubo = make_shared<UniformBuffer<GlobalUniformBuffer>>(l_device, gubo_config);

    // Create the descriptor set
    std::vector<shared_ptr<DescriptorElement>> elements;
    elements.push_back(gubo);

    unique_ptr<DescriptorSet> descriptor = make_unique<DescriptorSet>(l_device, elements);

    // Create the drawable collection
    unique_ptr<DrawableCollection> cube_collection = make_unique<DrawableCollection>(l_device, move(descriptor), command_pool->getCommandPool(), shaders);
    cube_collection->addElement(make_shared<Cube>());
    cube_collection->allocate();

    // Create the pipeline
    PipelineConfiguration config{};
    cube_pipeline = make_shared<Pipeline>(l_device, move(cube_collection), render_pass->getDepthTestType(), render_pass->getRenderPass(), config);

    // Add the pipeline to the renderer
    renderer->addPipeline(cube_pipeline);
}

void updateUniformBuffer()
{
    GlobalUniformBuffer buf{};

    // Make a rotation of the cube
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    // Set the gubo depending on the camera
    buf.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.f), glm::vec3(0, 1, 0));
    buf.view = camera.getLookAtMatrix();
    buf.projection = camera.getPerspectiveMatrix(window->getWidth(), window->getHeight());

    // Copy the gubo in memory
    gubo->setData(buf);
}

void onUpdate()
{
    updateUniformBuffer();

    // Draw the frame
    renderer->draw({0.0f, 0.0f, 0.0f, 1.0f});
}

void onClose()
{
    l_device->waitIdle();
}

void onUpdateSize()
{
    renderer->manageResize(window);
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
    l_device = make_shared<LogicalDevice>(move(pDevice), surface->getSurface());

    // Create command buffer and pool
    command_pool = make_shared<CommandPool>(l_device, surface->getSurface());
    command_buffer = make_unique<CommandBuffer>(l_device, command_pool->getCommandPool());

    // Create frame buffer collection
    swap_chain = make_unique<SwapChain>(l_device, window, surface->getSurface(), SwapChainConfiguration{});
    render_pass = make_unique<RenderPass>(l_device, swap_chain->getExtent(), swap_chain->getFormat(), DepthTestType::DEPTH_32);
    frame_buffer_collection = make_unique<FrameBufferCollection>(l_device, swap_chain->getImageViews(), swap_chain->getExtent(),
                                                                 render_pass->getDepthTestType(), render_pass->getDepthImageView(), render_pass->getRenderPass());

    // Create the renderer object
    renderer = make_unique<DefaultRenderer>();

    createGraphicsObjects();

    // Select the graphical objects
    renderer->selectInstance(vulkan);
    renderer->selectSurface(move(surface));
    renderer->selectLogicalDevice(l_device);
    renderer->selectSwapChain(move(swap_chain));
    renderer->selectRenderPass(move(render_pass));
    renderer->selectFrameBufferCollection(move(frame_buffer_collection));
    renderer->selectCommandBuffer(move(command_buffer));

    // Create imgui
    renderer->setupImGui(window->getWindow(), [&]() {});

    // Set camera position
    camera.setPosition({0, 2.0f, -10.0f});
    camera.lookAt({0, 0, 0});

    // Run the window
    window->run([=]()
                { onUpdate(); },
                [=]()
                { onUpdateSize(); },
                [=]()
                { onClose(); });

    return 0;
}