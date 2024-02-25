#include "defaultRenderer.h"
#include <chrono>

namespace framework
{
    DefaultRenderer::DefaultRenderer()
    {
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

        this->lDevice = d;

        // Create the sync objects
        imageAvailable = std::make_unique<Semaphore>(lDevice);
        renderFinished = std::make_unique<Semaphore>(lDevice);
        inFlight = std::make_unique<Fence>(lDevice, true);
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

        this->swapChain = std::move(s);
    }

    void DefaultRenderer::selectRenderPass(std::unique_ptr<RenderPass> r)
    {
        if (r == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null renderPass instance");
        }

        if (r->getRenderPass() == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[DefaultRenderer] Null render pass vulkan instance");
        }

        this->renderPass = std::move(r);
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

        this->frameBufferCollection = std::move(c);
    }

    void DefaultRenderer::selectCommandBuffer(std::unique_ptr<CommandBuffer> b)
    {
        if (b == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null command buffer instance");
        }

        this->commandBuffer = std::move(b);
    }

    void DefaultRenderer::recordCommandBuffer(uint32_t index, VkClearValue clearColor)
    {
        if (renderPass == nullptr || frameBufferCollection == nullptr || swapChain == nullptr || commandBuffer == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] graphics objects before recording the command buffer");
        }

        if (index >= frameBufferCollection->getFrameBuffers().size())
        {
            throw std::runtime_error("[DefaultRenderer] Index >= of the maximum size");
        }

        commandBuffer->beginRecording();
        renderPass->begin(commandBuffer->getCommandBuffer(), frameBufferCollection->getFrameBuffers()[index], swapChain->getExtent(), clearColor);

        for (const std::shared_ptr<Pipeline> &pipeline : pipelines)
        {
            if (pipeline->isVisible())
            {
                // Bind the pipeline (TODO make the compute pipeline also possible)
                vkCmdBindPipeline(commandBuffer->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());

                // Set dynamics of viewport and scissors
                VkViewport viewport{};

                viewport.x = 0;
                viewport.y = 0;
                viewport.width = static_cast<float>(swapChain->getExtent().width);
                viewport.height = static_cast<float>(swapChain->getExtent().height);
                viewport.minDepth = 0;
                viewport.maxDepth = 1;

                vkCmdSetViewport(commandBuffer->getCommandBuffer(), 0, 1, &viewport);

                VkRect2D scissors{};
                scissors.offset = {0, 0};
                scissors.extent = swapChain->getExtent();

                vkCmdSetScissor(commandBuffer->getCommandBuffer(), 0, 1, &scissors);

                // Bind the drawable collection of vertices
                VkBuffer vertexBuffers[] = {pipeline->getVertexBuffer()};
                VkDeviceSize offset[] = {0};
                vkCmdBindVertexBuffers(commandBuffer->getCommandBuffer(), 0, 1, vertexBuffers, offset);

                // Bind the drawable collection of indices
                vkCmdBindIndexBuffer(commandBuffer->getCommandBuffer(), pipeline->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

                // Bind the Uniform buffer and texture
                if (pipeline->hasDescriptorSet())
                {
                    vkCmdBindDescriptorSets(commandBuffer->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout(), 0, 1, &pipeline->getDescriptorSet(), 0, nullptr);
                }

                // Draw command (TODO make the vertices and instances configurable)
                vkCmdDrawIndexed(commandBuffer->getCommandBuffer(), pipeline->getIndexSize(), pipeline->getNumberOfInstances(), 0, 0, 0);
            }
        }

        // If present record also ImGui
        if (imGuiActive)
        {
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer->getCommandBuffer());
        }

        // Finish stopping the buffer recording
        renderPass->end(commandBuffer->getCommandBuffer());
        commandBuffer->stopRecording();
    }

    VkResult DefaultRenderer::draw(VkClearValue clearColor)
    {
        using clock = std::chrono::steady_clock;
        using micros = std::chrono::microseconds;

        // Start time for entire draw function
        auto startDraw = clock::now();

        // Start time for fence wait function
        auto start = clock::now();
        // Wait for previous frame to be completed
        inFlight->waitFor(1);

        // Record fence wait time
        timings.timeToWaitFence = std::chrono::duration_cast<micros>(clock::now() - start).count() / 1000.f;

        // Start time for pipeline updates
        start = clock::now();
        for (const std::shared_ptr<Pipeline> &pipeline : pipelines)
        {
            // Update the pipeline drawable collection
            pipeline->updateCollection();
        }

        // Record pipeline update time
        timings.timeToUpdatePipelines = std::chrono::duration_cast<micros>(clock::now() - start).count() / 1000.f;

        // Acquire image from swap chain
        uint32_t imageIndex;

        // Start time for acquiring the next image
        start = clock::now();

        // When the operation is complete the imageAvailable semaphore is signaled
        VkResult result = vkAcquireNextImageKHR(lDevice->getDevice(), swapChain->getSwapChain(), UINT64_MAX, imageAvailable->getSemaphore(), VK_NULL_HANDLE, &imageIndex);

        // Record time to acquire image
        timings.timeToAcquireImage = std::chrono::duration_cast<micros>(clock::now() - start).count() / 1000.f;

        // Check if the swapchain has become obsolete
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return result;
        }

        // Reset the fence
        inFlight->reset(1);

        // Reset the command buffer
        vkResetCommandBuffer(commandBuffer->getCommandBuffer(), 0);

        // Recreate the new frame if gui is active
        if (imGuiActive)
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Call the user defined gui descriptor
            if (guiDescriptor)
            {
                // Start time for describing the gui
                start = clock::now();
                guiDescriptor();

                // Record time to describe the gui
                timings.timeToDescribeGui = std::chrono::duration_cast<micros>(clock::now() - start).count() / 1000.f;
            }

            // Create the rendered frame described by user
            ImGui::Render();
        }

        // Start time for recording command buffer
        start = clock::now();

        // Record the buffer
        recordCommandBuffer(imageIndex, clearColor);

        // Record time to record command buffer
        timings.timeToRecordCommandBuffer = std::chrono::duration_cast<micros>(clock::now() - start).count() / 1000.f;

        // Submit the command buffer to the graphics queue
        VkSubmitInfo submitInfo{};

        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailable->getSemaphore()};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer->getCommandBuffer();

        VkSemaphore signalSemaphores[] = {renderFinished->getSemaphore()};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(lDevice->getGraphicsQueue(), 1, &submitInfo, inFlight->getFence()) != VK_SUCCESS)
        {
            throw std::runtime_error("[DefaultRenderer] Failed tu submit draw command buffer to graphics queue");
        }

        // Presentation (retrieve the rendering result)
        VkPresentInfoKHR presentInfo{};

        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores; // Wait for render

        VkSwapchainKHR swapChains[] = {swapChain->getSwapChain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        vkQueuePresentKHR(lDevice->getPresentQueue(), &presentInfo);

        // Record time to draw
        timings.timeToDraw = std::chrono::duration_cast<micros>(clock::now() - startDraw).count() / 1000.f;

        return VK_SUCCESS;
    }

    void DefaultRenderer::setupImGui(GLFWwindow *window, std::function<void()> guiDescriptor)
    {
        // Check that all the needed objects are present
        if (lDevice == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null logical device instance");
        }
        if (surface == nullptr)
        {
            throw std::runtime_error("[DefaultRenderer] Null surface instance");
        }
        if (swapChain == nullptr)
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

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags |= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(lDevice->getDevice(), &poolInfo, nullptr, &guiPool) != VK_SUCCESS)
        {
            throw std::runtime_error("[DefaultRenderer] Error creating descriptor pool for ImGui");
        }

        // Config all the necessary components for ImGui
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = vulkan->getInstance();
        initInfo.PhysicalDevice = lDevice->getPhysicalDevice()->getDevice();
        initInfo.Device = lDevice->getDevice();
        initInfo.QueueFamily = lDevice->findQueueFamilies(surface->getSurface()).graphicsFamily.value();
        initInfo.Queue = lDevice->getGraphicsQueue();
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPool = guiPool;
        initInfo.Allocator = nullptr;
        initInfo.MinImageCount = lDevice->getPhysicalDevice()->getSwapChainSupportDetails().capabilities.minImageCount;
        initInfo.ImageCount = swapChain->getImages().size();
        initInfo.CheckVkResultFn = nullptr;
        initInfo.RenderPass = renderPass->getRenderPass();

        ImGui_ImplVulkan_Init(&initInfo);

        // Active ImGui During the draw call
        imGuiActive = true;
        this->guiDescriptor = guiDescriptor;
    }

    void DefaultRenderer::manageResize(const std::shared_ptr<Window> &window)
    {
        // Wait that the device is ready
        lDevice->waitIdle();

        // Recreate swapchain and frame buffers
        swapChain->recreateSwapChain(window, surface->getSurface());
        renderPass->recreateRenderPass(swapChain->getExtent(), swapChain->getFormat());
        frameBufferCollection->recreateFrameBuffer(swapChain->getImageViews(), swapChain->getExtent(),
                                                   renderPass->getDepthTestType(), renderPass->getDepthImageView(), renderPass->getRenderPass());
    }
}