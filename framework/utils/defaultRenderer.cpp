#include "defaultRenderer.h"
#include <chrono>

namespace framework
{
    DefaultRenderer::DefaultRenderer()
    {
    }

    DefaultRenderer::~DefaultRenderer()
    {
        if (im_gui_active)
        {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImPlot::DestroyContext();
            ImGui::DestroyContext();

            // Delete custom ImGUI descriptor pool
            vkDestroyDescriptorPool(l_device->getDevice(), gui_pool, nullptr);
        }
    }

    void DefaultRenderer::selectInstance(const std::shared_ptr<Vulkan> &v)
    {
        if (v == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null vulkan pointer");
        }

        if (v->getInstance() == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[DefaultRenderer] Null vulkan instance");
        }

        this->vulkan = v;
    }

    void DefaultRenderer::selectSurface(std::unique_ptr<WindowSurface> s)
    {
        if (s == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null surface instance");
        }

        // Check that the surface is not null
        if (s->getSurface() == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[DefaultRenderer] Surface not created");
        }
        this->surface = std::move(s);
    }

    void DefaultRenderer::selectLogicalDevice(const std::shared_ptr<LogicalDevice> &d)
    {
        if (d == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null logical device pointer");
        }

        // TODO Check physical device is the same
        if (d->getDevice() == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[DefaultRenderer] Device not created");
        }

        this->l_device = d;

        // Create the sync objects
        image_available = std::make_unique<Semaphore>(l_device);
        render_finished = std::make_unique<Semaphore>(l_device);
        in_flight = std::make_unique<Fence>(l_device, true);
    }

    void DefaultRenderer::selectSwapChain(std::unique_ptr<SwapChain> s)
    {
        if (s == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null swapchain instance");
        }

        if (s->getSwapChain() == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[DefaultRenderer] Swapchain not created");
        }

        this->swap_chain = std::move(s);
    }

    void DefaultRenderer::selectRenderPass(std::unique_ptr<RenderPass> r)
    {
        if (r == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null render_pass instance");
        }

        if (r->getRenderPass() == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[DefaultRenderer] Null render pass vulkan instance");
        }

        this->render_pass = std::move(r);
    }

    void DefaultRenderer::addPipeline(std::shared_ptr<Pipeline> p)
    {
        if (p == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null pipeline instance");
        }

        // TODO check if not already present
        this->pipelines.push_back(std::move(p));
    }

    void DefaultRenderer::selectFrameBufferCollection(std::unique_ptr<FrameBufferCollection> c)
    {
        if (c == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null frame buffer collection instance");
        }

        this->frame_buffer_collection = std::move(c);
    }

    void DefaultRenderer::selectCommandBuffer(std::unique_ptr<CommandBuffer> b)
    {
        if (b == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null command buffer instance");
        }

        this->command_buffer = std::move(b);
    }

    void DefaultRenderer::recordCommandBuffer(uint32_t index, VkClearValue clear_color)
    {
        if (render_pass == nullptr || frame_buffer_collection == nullptr || swap_chain == nullptr || command_buffer == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] graphics objects before recording the command buffer");
        }

        if (index >= frame_buffer_collection->getFrameBuffers().size())
        {
            throw std::runtime_error("[DefaultRenderer] Index >= of the maximum size");
        }

        command_buffer->beginRecording();
        render_pass->begin(command_buffer->getCommandBuffer(), frame_buffer_collection->getFrameBuffers()[index], swap_chain->getExtent(), clear_color);

        for (const std::shared_ptr<Pipeline> &pipeline : pipelines)
        {
            if (pipeline->isVisible())
            {
                // Bind the pipeline (TODO make the compute pipeline also possible)
                vkCmdBindPipeline(command_buffer->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());

                // Set dynamics of viewport and scissors
                VkViewport viewport{};

                viewport.x = 0;
                viewport.y = 0;
                viewport.width = static_cast<float>(swap_chain->getExtent().width);
                viewport.height = static_cast<float>(swap_chain->getExtent().height);
                viewport.minDepth = 0;
                viewport.maxDepth = 1;

                vkCmdSetViewport(command_buffer->getCommandBuffer(), 0, 1, &viewport);

                VkRect2D scissors{};
                scissors.offset = {0, 0};
                scissors.extent = swap_chain->getExtent();

                vkCmdSetScissor(command_buffer->getCommandBuffer(), 0, 1, &scissors);

                // Bind the drawable collection of vertices
                VkBuffer vertex_buffers[] = {pipeline->getVertexBuffer()};
                VkDeviceSize offset[] = {0};
                vkCmdBindVertexBuffers(command_buffer->getCommandBuffer(), 0, 1, vertex_buffers, offset);

                // Bind the drawable collection of indices
                vkCmdBindIndexBuffer(command_buffer->getCommandBuffer(), pipeline->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

                // Bind the Uniform buffer and texture
                if (pipeline->hasDescriptorSet())
                {
                    vkCmdBindDescriptorSets(command_buffer->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout(), 0, 1, &pipeline->getDescriptorSet(), 0, nullptr);
                }

                // Draw command (TODO make the vertices and instances configurable)
                vkCmdDrawIndexed(command_buffer->getCommandBuffer(), pipeline->getIndexSize(), pipeline->getNumberOfInstances(), 0, 0, 0);
            }
        }

        // If present record also ImGui
        if (im_gui_active)
        {
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer->getCommandBuffer());
        }

        // Finish stopping the buffer recording
        render_pass->end(command_buffer->getCommandBuffer());
        command_buffer->stopRecording();
    }

    VkResult DefaultRenderer::draw(VkClearValue clear_color)
    {
        using clock = std::chrono::steady_clock;
        using micros = std::chrono::microseconds;

        // Start time for entire draw function
        auto start_draw = clock::now();

        // Start time for fence wait function
        auto start = clock::now();
        // Wait for previous frame to be completed
        in_flight->waitFor(1);

        // Record fence wait time
        timings.time_to_wait_fence = std::chrono::duration_cast<micros>(clock::now() - start).count() / 1000.f;

        // Start time for pipeline updates
        start = clock::now();
        for (const std::shared_ptr<Pipeline> &pipeline : pipelines)
        {
            // Update the pipeline drawable collection
            pipeline->updateCollection();
        }

        // Record pipeline update time
        timings.time_to_update_pipelines = std::chrono::duration_cast<micros>(clock::now() - start).count() / 1000.f;

        // Acquire image from swap chain
        uint32_t image_index;

        // Start time for acquiring the next image
        start = clock::now();

        // When the operation is complete the image_available semaphore is signaled
        VkResult result = vkAcquireNextImageKHR(l_device->getDevice(), swap_chain->getSwapChain(), UINT64_MAX, image_available->getSemaphore(), VK_NULL_HANDLE, &image_index);

        // Record time to acquire image
        timings.time_to_acquire_image = std::chrono::duration_cast<micros>(clock::now() - start).count() / 1000.f;

        // Check if the swapchain has become obsolete
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return result;
        }

        // Reset the fence
        in_flight->reset(1);

        // Reset the command buffer
        vkResetCommandBuffer(command_buffer->getCommandBuffer(), 0);

        // Recreate the new frame if gui is active
        if (im_gui_active)
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Call the user defined gui descriptor
            if (gui_descriptor)
            {
                // Start time for describing the gui
                start = clock::now();
                gui_descriptor();

                // Record time to describe the gui
                timings.time_to_describe_gui = std::chrono::duration_cast<micros>(clock::now() - start).count() / 1000.f;
            }

            // Create the rendered frame described by user
            ImGui::Render();
        }

        // Start time for recording command buffer
        start = clock::now();

        // Record the buffer
        recordCommandBuffer(image_index, clear_color);

        // Record time to record command buffer
        timings.time_to_record_command_buffer = std::chrono::duration_cast<micros>(clock::now() - start).count() / 1000.f;

        // Submit the command buffer to the graphics queue
        VkSubmitInfo submit_info{};

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore wait_semaphores[] = {image_available->getSemaphore()};
        VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = wait_semaphores;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer->getCommandBuffer();

        VkSemaphore signalSemaphores[] = {render_finished->getSemaphore()};
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(l_device->getGraphicsQueue(), 1, &submit_info, in_flight->getFence()) != VK_SUCCESS)
        {
            throw std::runtime_error("[DefaultRenderer] Failed tu submit draw command buffer to graphics queue");
        }

        // Presentation (retrieve the rendering result)
        VkPresentInfoKHR present_info{};

        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signalSemaphores; // Wait for render

        VkSwapchainKHR swapChains[] = {swap_chain->getSwapChain()};
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swapChains;
        present_info.pImageIndices = &image_index;
        present_info.pResults = nullptr;

        vkQueuePresentKHR(l_device->getPresentQueue(), &present_info);

        // Record time to draw
        timings.time_to_draw = std::chrono::duration_cast<micros>(clock::now() - start_draw).count() / 1000.f;

        return VK_SUCCESS;
    }

    void DefaultRenderer::setupImGui(GLFWwindow *window, std::function<void()> gui_descriptor)
    {
        // Check that all the needed objects are present
        if (l_device == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null logical device instance");
        }
        if (surface == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null surface instance");
        }
        if (swap_chain == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null swapchain instance");
        }

        // Create the ImGui context and internal state
        ImGui::CreateContext();
        ImPlot::CreateContext();

        // Configure the InputOutput
        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->AddFontDefault();
        io.Fonts->Build();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Setup ImGui for the style (TODO make it customizable)
        ImGui::StyleColorsClassic();

        // Setup the glfw window to blindly bind the callbacks
        ImGui_ImplGlfw_InitForVulkan(window, true);

        // Create ad-doc descriptor pool (TODO make it with a descriptor element)
        std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags |= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        pool_info.pPoolSizes = poolSizes.data();
        pool_info.maxSets = 1;

        if (vkCreateDescriptorPool(l_device->getDevice(), &pool_info, nullptr, &gui_pool) != VK_SUCCESS)
        {
            throw std::runtime_error("[DefaultRenderer] Error creating descriptor pool for ImGui");
        }

        // Config all the necessary components for ImGui
        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance = vulkan->getInstance();
        init_info.PhysicalDevice = l_device->getPhysicalDevice()->getDevice();
        init_info.Device = l_device->getDevice();
        init_info.QueueFamily = l_device->findQueueFamilies(surface->getSurface()).graphics_family.value();
        init_info.Queue = l_device->getGraphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = gui_pool;
        init_info.Allocator = nullptr;
        init_info.MinImageCount = l_device->getPhysicalDevice()->getSwapChainSupportDetails().capabilities.minImageCount;
        init_info.ImageCount = swap_chain->getImages().size();
        init_info.CheckVkResultFn = nullptr;
        init_info.RenderPass = render_pass->getRenderPass();

        ImGui_ImplVulkan_Init(&init_info);

        // Active ImGui During the draw call
        im_gui_active = true;
        this->gui_descriptor = gui_descriptor;
    }

    void DefaultRenderer::manageResize(const std::shared_ptr<Window> &window)
    {
        // Wait that the device is ready
        l_device->waitIdle();

        // Recreate swapchain and frame buffers
        swap_chain->recreateSwapChain(window, surface->getSurface());
        render_pass->recreateRenderPass(swap_chain->getExtent(), swap_chain->getFormat());
        frame_buffer_collection->recreateFrameBuffer(swap_chain->getImageViews(), swap_chain->getExtent(),
                                                     render_pass->getDepthTestType(), render_pass->getDepthImageView(), render_pass->getRenderPass());
    }
}