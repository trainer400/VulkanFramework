#pragma once

#include <devices/logicalDevice.h>
#include <core/renderPass.h>
#include <core/shader.h>
#include <core/drawableCollection.h>
#include <core/descriptorSet.h>

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace framework
{
    // TODO expand this struct
    struct PipelineConfiguration
    {
        VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
        VkCullModeFlagBits cull_mode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;
    };

    class Pipeline
    {
    public:
        Pipeline(const std::shared_ptr<LogicalDevice> &l_device,
                 std::unique_ptr<DrawableCollection> drawable_collection,
                 const DepthTestType &depth_test_type, const VkRenderPass &render_pass,
                 const PipelineConfiguration &config);
        ~Pipeline();

        /**
         * @brief Updates the collection inside the GPU memory if necessary
         */
        inline void updateCollection() { collection->updateElements(); }

        // Getters
        const VkPipeline &getPipeline() { return pipeline; }
        const VkPipelineLayout &getLayout() { return layout; }
        const VkBuffer &getVertexBuffer() { return collection->getVertexBuffer(); }
        const VkBuffer &getIndexBuffer() { return collection->getIndexBuffer(); }
        const VkDescriptorSet &getDescriptorSet() { return collection->getDescriptorSet(); }
        uint32_t getVerticesNumber() { return collection->getVerticesNumber(); }
        uint32_t getIndexSize() { return collection->getIndexSize(); }
        uint32_t getNumberOfInstances() { return collection->getNumberOfInstances(); }
        bool isVisible() { return visible; }
        bool hasDescriptorSet() { return collection->hasDescriptorSet(); }

        // Setters
        void setVisibility(bool v) { visible = v; }

    private:
        bool visible = true;

        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;

        std::shared_ptr<LogicalDevice> l_device;
        std::unique_ptr<DrawableCollection> collection;
    };
}