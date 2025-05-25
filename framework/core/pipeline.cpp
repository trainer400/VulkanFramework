#include "pipeline.h"
#include <stdexcept>

namespace framework
{
    Pipeline::Pipeline(const std::shared_ptr<LogicalDevice> &l_device,
                       std::unique_ptr<DrawableCollection> drawable_collection,
                       const DepthTestType &depth_test_type, const VkRenderPass &render_pass,
                       const PipelineConfiguration &config)
    {
        if (l_device == nullptr)
        {
            throw std::runtime_error("[Pipeline] Null device instance");
        }

        if (render_pass == nullptr)
        {
            throw std::runtime_error("[Pipeline] Null render pass instance");
        }

        if (drawable_collection == nullptr)
        {
            throw std::runtime_error("[Pipeline] Null drawable collection");
        }

        if (drawable_collection->getAttributeDescriptions().size() == 0)
        {
            throw std::runtime_error("[Pipeline] Empty drawable collection");
        }

        this->l_device = l_device;
        this->collection = std::move(drawable_collection);

        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

        // In case of a depth buffer
        VkPipelineDepthStencilStateCreateInfo depth_stencil{};

        // For every shader create its stage
        for (std::shared_ptr<Shader> shader : collection->getShaders())
        {
            VkPipelineShaderStageCreateInfo createInfo{};

            // Configure the shader depending on its type
            createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            createInfo.stage = shader->getShaderStage();
            createInfo.module = shader->getShader();
            createInfo.pName = "main";

            // Push the created info into the structure
            shader_stages.push_back(createInfo);
        }

        // Add the dynamic state (TODO make this configurable)
        std::vector<VkDynamicState> dynamic_states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamic_state{};

        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();

        // Vertex input buffer (TODO configure this to have a mutable vertex buffer)
        VkPipelineVertexInputStateCreateInfo vertex_info{};

        VkVertexInputBindingDescription binding_description = collection->getBindingDescription();
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions = collection->getAttributeDescriptions();

        vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_info.vertexBindingDescriptionCount = 1;
        vertex_info.pVertexBindingDescriptions = &binding_description;
        vertex_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
        vertex_info.pVertexAttributeDescriptions = attribute_descriptions.data();

        // Input assembly (TODO make this configurable)
        VkPipelineInputAssemblyStateCreateInfo input_assembly{};

        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        // Viewport and scissors
        VkPipelineViewportStateCreateInfo viewport_state{};

        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        // Rasterizer (TODO make this configurable)
        VkPipelineRasterizationStateCreateInfo rasterizer{};

        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = config.polygon_mode;
        rasterizer.cullMode = config.cull_mode;
        rasterizer.frontFace = config.front_face;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0;
        rasterizer.depthBiasClamp = 0;
        rasterizer.depthBiasSlopeFactor = 0;
        rasterizer.lineWidth = 1.0f;

        // Multi-sampling (TODO evaluate to make this configurable)
        VkPipelineMultisampleStateCreateInfo multisampling{};

        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        // Color blending (TODO evaluate to make this configurable)
        VkPipelineColorBlendAttachmentState color_blend_attachment{};

        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = VK_TRUE;
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo color_blending{};

        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.logicOp = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &color_blend_attachment;
        color_blending.blendConstants[0] = 0.0f;
        color_blending.blendConstants[1] = 0.0f;
        color_blending.blendConstants[2] = 0.0f;
        color_blending.blendConstants[3] = 0.0f;

        // Pipeline layout (aka Uniforms TODO make this configurable)
        VkPipelineLayoutCreateInfo layout_info{};

        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = 0;
        layout_info.pSetLayouts = nullptr;
        layout_info.pushConstantRangeCount = 0;
        layout_info.pPushConstantRanges = nullptr;

        if (collection->hasDescriptorSet())
        {
            layout_info.setLayoutCount = 1;
            layout_info.pSetLayouts = &collection->getDescriptorSetLayout();
        }

        if (vkCreatePipelineLayout(l_device->getDevice(), &layout_info, nullptr, &layout) != VK_SUCCESS)
        {
            throw std::runtime_error("[Pipeline] Error creating pipeline layout");
        }

        // Create the pipeline
        VkGraphicsPipelineCreateInfo pipeline_info{};

        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = static_cast<uint32_t>(collection->getShaders().size());
        pipeline_info.pStages = shader_stages.data();
        pipeline_info.pVertexInputState = &vertex_info;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = nullptr;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.layout = layout;
        pipeline_info.renderPass = render_pass;
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        // Check if depth testing is enabled
        if (depth_test_type != NONE)
        {
            depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil.depthTestEnable = VK_TRUE;
            depth_stencil.depthWriteEnable = VK_TRUE;
            depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depth_stencil.depthBoundsTestEnable = VK_FALSE;
            depth_stencil.stencilTestEnable = VK_FALSE;

            pipeline_info.pDepthStencilState = &depth_stencil;
        }

        if (vkCreateGraphicsPipelines(l_device->getDevice(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("[Pipeline] Error creating the pipeline");
        }
    }

    Pipeline::~Pipeline()
    {
        if (layout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(l_device->getDevice(), layout, nullptr);
        }

        if (pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(l_device->getDevice(), pipeline, nullptr);
        }
    }
}