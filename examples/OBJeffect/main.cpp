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
shared_ptr<LogicalDevice> l_device;
shared_ptr<CommandPool> command_pool;
unique_ptr<CommandBuffer> command_buffer;
shared_ptr<Pipeline> obj_pipeline;
unique_ptr<SwapChain> swap_chain;
unique_ptr<RenderPass> render_pass;
unique_ptr<FrameBufferCollection> frame_buffer_collection;
unique_ptr<DefaultRenderer> renderer;
FPSCamera camera{100, 45, 0.1f, 10000.0f, {}};
bool mouseFix = true; // Sets the mouse position at the center

// Test variables
glm::vec3 plane_direction;
float color_magnitude;

struct GlobalUniformBuffer
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec3 spawn_plane_direction;
    alignas(4) float color_spawn_plane_direction;
};

// Uniform buffer
shared_ptr<UniformBuffer<GlobalUniformBuffer>> gubo;
shared_ptr<Texture> texture;

void createGraphicsObjects()
{
    shared_ptr<Shader> vertex = make_shared<Shader>(l_device, "examples/OBJeffect/shaders/vert.spv", ShaderType::VERTEX);
    shared_ptr<Shader> fragment = make_shared<Shader>(l_device, "examples/OBJeffect/shaders/frag.spv", ShaderType::FRAGMENT);

    // Read the shaders
    vector<shared_ptr<Shader>> shaders;
    shaders.push_back(vertex);
    shaders.push_back(fragment);

    // Create the uniform buffer
    UniformBufferConfiguration gubo_config;
    gubo_config.binding_index = 0;
    gubo_config.stage_flags = VK_SHADER_STAGE_ALL;
    gubo = make_shared<UniformBuffer<GlobalUniformBuffer>>(l_device, gubo_config);

    // Parse the obj file
    std::vector<std::string> textures;
    ObjectParserConfiguration parser_config;
    parser_config.add_medians = true;
    std::vector<std::shared_ptr<DefaultDrawableElement>> drawable_elements = parseObjFile("examples/OBJeffect/models/Rock_5.obj", parser_config, textures);

    // Create the texture
    texture = make_shared<Texture>(l_device, command_pool->getCommandPool(), textures[0].c_str(), 1);

    // Create the descriptor set
    std::vector<shared_ptr<DescriptorElement>> elements;
    elements.push_back(gubo);
    elements.push_back(texture);

    unique_ptr<DescriptorSet> descriptor = make_unique<DescriptorSet>(l_device, elements);

    // Create the drawable collection
    unique_ptr<DrawableCollection> object_collection = make_unique<DrawableCollection>(l_device, move(descriptor), command_pool->getCommandPool(), shaders);

    // Add the drawable elements
    for (auto &e : drawable_elements)
        object_collection->addElement(e);
    object_collection->allocate();

    // Create the pipeline
    PipelineConfiguration config{};
    config.cull_mode = VK_CULL_MODE_NONE;
    obj_pipeline = make_shared<Pipeline>(l_device, move(object_collection), render_pass->getDepthTestType(), render_pass->getRenderPass(), config);

    // Add the pipeline to the renderer
    renderer->addPipeline(obj_pipeline);
}

void updateUniformBuffer()
{
    GlobalUniformBuffer buf{};

    // Set the plane values
    buf.spawn_plane_direction = plane_direction;
    buf.color_spawn_plane_direction = color_magnitude;

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
    l_device->waitIdle();
}

void onUpdateSize()
{
    renderer->manageResize(window);
}

void setupGui()
{
    ImGui::Begin("Prova");

    // Create slider for spawn plane and magnitudes
    ImGui::DragFloat3("Plane direction", &plane_direction.x, 0.001f, -1.0f, 1.0f, "%.3f");
    ImGui::DragFloat("Color magnitude", &color_magnitude, 10.0f, -10000.0f, 10000.0f, "%.1f");

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

    // Create the object to render
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
    renderer->setupImGui(window->getWindow(), [&]()
                         { setupGui(); });

    // Setup camera
    camera.setPosition({0, 2.0f, -10.0f});
    camera.registerCallbacks(window);

    // Add a callback for ESC
    window->addKeyCallback(GLFW_KEY_ESCAPE, [&](int key, int action)
                           { if(action == GLFW_PRESS){mouseFix = !mouseFix;} });

    // Setup test variables
    plane_direction = {-1, 0, 0};
    color_magnitude = 1.f;

    // Run the window
    window->run([=]()
                { onUpdate(); },
                [=]()
                { onUpdateSize(); },
                [=]()
                { onClose(); });

    return 0;
}