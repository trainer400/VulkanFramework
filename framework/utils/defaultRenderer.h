#pragma once
#include <devices/physicalDevice.h>
#include <devices/logicalDevice.h>
#include <window/window.h>
#include <window/windowSurface.h>
#include <core/vulkan.h>
#include <core/swapChain.h>
#include <core/shader.h>
#include <core/renderPass.h>
#include <core/pipeline.h>
#include <core/frameBufferCollection.h>
#include <core/commandBuffer.h>
#include <core/commandPool.h>
#include <core/semaphore.h>
#include <core/fence.h>

#include <ImGui/imgui.h>
#include <ImGui/backends/imgui_impl_glfw.h>
#include <ImGui/backends/imgui_impl_vulkan.h>

#include <ImPlot/implot.h>

#include <memory>

namespace framework
{
    struct TimingMeasurement
    {
        float time_to_draw = 0;
        float time_to_wait_fence = 0;
        float time_to_update_pipelines = 0;
        float time_to_acquire_image = 0;
        float time_to_describe_gui = 0;
        float time_to_record_command_buffer = 0;
    };

    class DefaultRenderer
    {
    public:
        DefaultRenderer();
        ~DefaultRenderer();

        /**
         * @brief Sets the vulkan object
         */
        void
        selectInstance(const std::shared_ptr<Vulkan> &v);

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
        void recordCommandBuffer(uint32_t index, VkClearValue clear_color);

        /**
         * @brief Call to the command buffer to draw on screen the previously specified objects
         */
        VkResult draw(VkClearValue clear_color);

        /**
         * @brief Setups ImGui with the needed vulkan instances
         */
        void setupImGui(GLFWwindow *window, std::function<void()> gui_descriptor);

        /**
         * @brief Called after an invalidation of the swap-chain (usually occurs after a resize).
         * It recreates the swap chain and the frame buffer collection
         */
        void manageResize(const std::shared_ptr<Window> &window);

        // Getters
        const TimingMeasurement &getTimings() { return timings; }

    private:
        // Framework objects
        std::shared_ptr<Vulkan> vulkan;
        std::shared_ptr<LogicalDevice> l_device;
        std::unique_ptr<WindowSurface> surface;
        std::unique_ptr<SwapChain> swap_chain;
        std::unique_ptr<RenderPass> render_pass;
        std::vector<std::shared_ptr<Pipeline>> pipelines;
        std::unique_ptr<FrameBufferCollection> frame_buffer_collection;
        std::unique_ptr<CommandBuffer> command_buffer;
        std::unique_ptr<Semaphore> image_available;
        std::unique_ptr<Semaphore> render_finished;
        std::unique_ptr<Fence> in_flight;

        // Timing measurements
        TimingMeasurement timings;

        // ImGui
        bool im_gui_active = false;
        VkDescriptorPool gui_pool = VK_NULL_HANDLE;
        std::function<void()> gui_descriptor;
    };
}