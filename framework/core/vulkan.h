#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <optional>
#include <devices/physicalDevice.h>
#include <devices/logicalDevice.h>
#include <window/window.h>
#include <window/windowSurface.h>
#include <core/swapChain.h>
#include <core/shader.h>
#include <core/renderPass.h>
#include <core/pipeline.h>
#include <core/frameBufferCollection.h>
#include <core/commandBuffer.h>
#include <core/commandPool.h>
#include <core/semaphore.h>
#include <vector>
#include <memory>

#include <ImGui/imgui.h>
#include <ImGui/backends/imgui_impl_glfw.h>
#include <ImGui/backends/imgui_impl_vulkan.h>

#include <ImPlot/implot.h>

namespace framework
{
    struct TimingMeasurement
    {
        float timeToDraw = 0;
        float timeToWaitFence = 0;
        float timeToUpdatePipelines = 0;
        float timeToAcquireImage = 0;
        float timeToDescribeGui = 0;
        float timeToRecordCommandBuffer = 0;
    };

    class Vulkan
    {
    public:
        /**
         * @brief Construct a new Vulkan object
         *
         * @param applicationName The name to communicate to the driver
         * @param engineName The engine name to communicate to the driver
         * @param addedExtensions Extensions to add to the engine
         */
        Vulkan(const char *applicationName, const char *engineName, const std::vector<const char *> &addedExtensions, bool enableLayers = false);
        ~Vulkan();

        /**
         * @brief Sets the window surface, which is mandatory to find the queue family
         */
        void selectSurface(std::unique_ptr<WindowSurface> s);

        /**
         * @brief Sets the logical device to be used
         */
        void selectLogicalDevice(const std::shared_ptr<LogicalDevice> &d);

        /**
         * @brief Sets the swap chain to be used
         */
        void selectSwapChain(std::unique_ptr<SwapChain> s);

        /**
         * @brief Sets the render pass to be used
         */
        void selectRenderPass(std::unique_ptr<RenderPass> r);

        /**
         * @brief Sets the pipeline to be used in draw function
         */
        void addPipeline(std::shared_ptr<Pipeline> p);

        /**
         * @brief Sets the frame buffer collection to be used
         */
        void selectFrameBufferCollection(std::unique_ptr<FrameBufferCollection> c);

        /**
         * @brief Sets the command buffer
         * TODO: evaluate to make this method add command buffer
         */
        void selectCommandBuffer(std::unique_ptr<CommandBuffer> b);

        /**
         * @brief Records the command into the command buffer. The index is the swap chain used one
         */
        void recordCommandBuffer(uint32_t index, VkClearValue clearColor);

        /**
         * @brief Creates the two semaphores and the fence needed to make the CPU wait for rendering action to be completed
         * and the GPU wait for image available and render finish.
         */
        void createSyncObjects();

        /**
         * @brief Call to the command buffer to draw on screen the previously specified objects
         */
        VkResult draw(VkClearValue clearColor);

        /**
         * @brief Setups ImGui with the needed vulkan instances
         */
        void setupImGui(GLFWwindow *window, std::function<void()> guiDescriptor);

        /**
         * @brief Called after an invalidation of the swap-chain (usually occurs after a resize).
         * It recreates the swap chain and the frame buffer collection
         */
        void manageResize(const std::shared_ptr<Window> &window);

        /**
         * @brief Get the Instance object
         */
        const VkInstance &getInstance() { return instance; }
        const TimingMeasurement &getTimings() { return timings; }

    private:
        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

        // Vulkan objects
        VkInstance instance = VK_NULL_HANDLE;

        // Sync objects
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence inFlightFence = VK_NULL_HANDLE;

        // Framework objects
        std::shared_ptr<LogicalDevice> lDevice;
        std::unique_ptr<WindowSurface> surface;
        std::unique_ptr<SwapChain> swapChain;
        std::unique_ptr<RenderPass> renderPass;
        std::vector<std::shared_ptr<Pipeline>> pipelines;
        std::unique_ptr<FrameBufferCollection> frameBufferCollection;
        std::unique_ptr<CommandBuffer> commandBuffer;
        std::unique_ptr<Semaphore> imageAvailable;
        std::unique_ptr<Semaphore> renderFinished;

        // Timing measurements
        TimingMeasurement timings;

        // ImGui
        bool imGuiActive = false;
        VkDescriptorPool guiPool = VK_NULL_HANDLE;
        std::function<void()> guiDescriptor;
    };
}