#include "vulkan.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <set>
#include <cstring>
#include <chrono>
#include <iostream>

namespace framework
{
    Vulkan::Vulkan(const char *applicationName, const char *engineName, const std::vector<const char *> &addedExtensions, bool enableLayers)
    {
        if (applicationName == nullptr || engineName == nullptr)
        {
            throw std::runtime_error("[Vulkan] Nullptrs in application and engine names");
        }

        const char **glfwExtensions;
        uint32_t glfwExtensionCount = 0;

        // Gather the glfw info
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        if (glfwExtensions == nullptr)
        {
            throw std::runtime_error("[Vulkan] Impossible to enumerate instance extensions");
        }

        // Merge the extension names
        char **extensions;
        extensions = new char *[glfwExtensionCount + addedExtensions.size()];

        for (int i = 0; i < glfwExtensionCount; i++)
        {
            extensions[i] = new char[strlen(glfwExtensions[i]) + 1];
            strcpy(extensions[i], glfwExtensions[i]);
        }

        for (int i = 0; i < addedExtensions.size(); i++)
        {
            extensions[glfwExtensionCount + i] = new char[strlen(addedExtensions[i]) + 1];
            strcpy(extensions[glfwExtensionCount + i], addedExtensions[i]);
        }

        // Setup the structure to instantiate vulkan
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = applicationName;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = engineName;
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = glfwExtensionCount + addedExtensions.size();
        createInfo.ppEnabledExtensionNames = extensions;
        createInfo.enabledLayerCount = 0;

        // Create validation layers
        if (enableLayers)
        {
            // Enumerate valiation layers
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers;
            availableLayers.resize(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            // Check validation layers support
            bool layerFound = false;
            for (const char *layerName : validationLayers)
            {
                layerFound = false;
                for (const VkLayerProperties properties : availableLayers)
                {
                    if (strcmp(layerName, properties.layerName) == 0)
                    {
                        layerFound = true;

                        // Found the layer
                        break;
                    }
                }

                // In case of layer not found break
                if (!layerFound)
                {
                    throw std::runtime_error("[Vulkan] Not compatible validation layer found");
                }
            }

            // Set the validation layers
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }

        // Create vulkan instance
        if (vkCreateInstance(&createInfo, nullptr, &this->instance) != VK_SUCCESS)
        {
            throw std::runtime_error("[Vulkan] Impossible to create VK instance");
        }

        // After instance creation free the extensions
        for (int i = 0; i < glfwExtensionCount + addedExtensions.size(); i++)
        {
            delete (extensions[i]);
        }
        delete extensions;
    }

    Vulkan::~Vulkan()
    {
        if (lDevice != nullptr)
        {
            if (imageAvailableSemaphore != VK_NULL_HANDLE &&
                renderFinishedSemaphore != VK_NULL_HANDLE &&
                inFlightFence != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(lDevice->getDevice(), imageAvailableSemaphore, nullptr);
                vkDestroySemaphore(lDevice->getDevice(), renderFinishedSemaphore, nullptr);
                vkDestroyFence(lDevice->getDevice(), inFlightFence, nullptr);
            }
        }

        if (imGuiActive)
        {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImPlot::DestroyContext();
            ImGui::DestroyContext();

            // Delete custom ImGUI descriptor pool
            vkDestroyDescriptorPool(lDevice->getDevice(), guiPool, nullptr);
        }
        // vkDestroyInstance(instance, nullptr);
    }

    void Vulkan::selectSurface(std::unique_ptr<WindowSurface> s)
    {
        if (s == nullptr)
        {
            throw std::runtime_error("[Vulkan] Null surface instance");
        }

        // Check that the surface is not null
        if (s->getSurface() == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[Vulkan] Surface not created");
        }
        this->surface = std::move(s);
    }

    void Vulkan::selectLogicalDevice(const std::shared_ptr<LogicalDevice> &d)
    {
        if (d == nullptr)
        {
            throw std::runtime_error("[Vulkan] Null logical device pointer");
        }

        // TODO Check physical device is the same
        if (d->getDevice() == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[Vulkan] Device not created");
        }

        this->lDevice = d;
    }

    void Vulkan::selectSwapChain(std::unique_ptr<SwapChain> s)
    {
        if (s == nullptr)
        {
            throw std::runtime_error("[Vulkan] Null swapchain instance");
        }

        if (s->getSwapChain() == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[Vulkan] Swapchain not created");
        }

        this->swapChain = std::move(s);
    }

    void Vulkan::selectRenderPass(std::unique_ptr<RenderPass> r)
    {
        if (r == nullptr)
        {
            throw std::runtime_error("[Vulkan] Null renderPass instance");
        }

        if (r->getRenderPass() == VK_NULL_HANDLE)
        {
            throw std::runtime_error("[Vulkan] Null render pass vulkan instance");
        }

        this->renderPass = std::move(r);
    }

    void Vulkan::addPipeline(std::shared_ptr<Pipeline> p)
    {
        if (p == nullptr)
        {
            throw std::runtime_error("[Vulkan] Null pipeline instance");
        }

        // TODO check if not already present
        this->pipelines.push_back(std::move(p));
    }

    void Vulkan::selectFrameBufferCollection(std::unique_ptr<FrameBufferCollection> c)
    {
        if (c == nullptr)
        {
            throw std::runtime_error("[Vulkan] Null frame buffer collection instance");
        }

        this->frameBufferCollection = std::move(c);
    }

    void Vulkan::selectCommandBuffer(std::unique_ptr<CommandBuffer> b)
    {
        if (b == nullptr)
        {
            throw std::runtime_error("[Vulkan] Null command buffer instance");
        }

        this->commandBuffer = std::move(b);
    }

    void Vulkan::recordCommandBuffer(uint32_t index, VkClearValue clearColor)
    {
        if (renderPass == nullptr || frameBufferCollection == nullptr || swapChain == nullptr || commandBuffer == nullptr)
        {
            throw std::runtime_error("[Vulkan] graphics objects before recording the command buffer");
        }

        if (index >= frameBufferCollection->getFrameBuffers().size())
        {
            throw std::runtime_error("[Vulkan] Index >= of the maximum size");
        }

        commandBuffer->beginRecording();

        // Start the render pass
        VkRenderPassBeginInfo renderPassInfo{};

        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass->getRenderPass();
        renderPassInfo.framebuffer = frameBufferCollection->getFrameBuffers()[index];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChain->getExtent();

        // TODO make clear color configurable
        std::vector<VkClearValue> clearValues;
        clearValues.push_back(clearColor);
        clearValues.push_back({1.0f, 0.0f}); // Depth and stencil

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer->getCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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
                if (pipeline->getDescriptorSet() != nullptr)
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

        // End the render pass
        vkCmdEndRenderPass(commandBuffer->getCommandBuffer());

        // Finish stopping the buffer recording
        commandBuffer->stopRecording();
    }

    void Vulkan::createSyncObjects()
    {
        if (lDevice == nullptr)
        {
            throw std::runtime_error("[Vulkan] Logical device has not been created yet");
        }

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        // Create the fence with the first state signaled (to make the draw method go ahead the first time)
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(lDevice->getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(lDevice->getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(lDevice->getDevice(), &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("[Vulkan] Impossible to create sync objects");
        }
    }

    VkResult Vulkan::draw(VkClearValue clearColor)
    {
        using clock = std::chrono::steady_clock;
        using micros = std::chrono::microseconds;

        // Start time for entire draw function
        auto startDraw = clock::now();

        // Start time for fence wait function
        auto start = clock::now();
        // Wait for previous frame to be completed
        vkWaitForFences(lDevice->getDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);

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
        VkResult result = vkAcquireNextImageKHR(lDevice->getDevice(), swapChain->getSwapChain(), UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        // Record time to acquire image
        timings.timeToAcquireImage = std::chrono::duration_cast<micros>(clock::now() - start).count() / 1000.f;

        // Check if the swapchain has become obsolete
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return result;
        }

        // Reset the fence
        vkResetFences(lDevice->getDevice(), 1, &inFlightFence);

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

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer->getCommandBuffer();

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(lDevice->getGraphicsQueue(), 1, &submitInfo, inFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("[Vulkan] Failed tu submit draw command buffer to graphics queue");
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

    void Vulkan::setupImGui(GLFWwindow *window, std::function<void()> guiDescriptor)
    {
        // Check that all the needed objects are present
        if (lDevice == nullptr)
        {
            throw std::runtime_error("[Vulkan] Null logical device instance");
        }
        if (surface == nullptr)
        {
            throw std::runtime_error("[Vulkan] Null surface instance");
        }
        if (swapChain == nullptr)
        {
            throw std::runtime_error("[Vulkan] Null swapchain instance");
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
            throw std::runtime_error("[Vulkan] Error creating descriptor pool for ImGui");
        }

        // Config all the necessary components for ImGui
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = instance;
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

    void Vulkan::manageResize(const std::shared_ptr<Window> &window)
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