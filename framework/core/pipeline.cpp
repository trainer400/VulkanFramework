#include "pipeline.h"
#include <stdexcept>

namespace framework
{
    Pipeline::Pipeline(const std::shared_ptr<LogicalDevice> &lDevice,
                       std::unique_ptr<DrawableCollection> drawableCollection,
                       const DepthTestType &depthTestType, const VkRenderPass &renderPass,
                       const PipelineConfiguration &config)
    {
        if (lDevice == nullptr)
        {
            throw std::runtime_error("[Pipeline] Null device instance");
        }

        if (renderPass == nullptr)
        {
            throw std::runtime_error("[Pipeline] Null render pass instance");
        }

        if (drawableCollection == nullptr)
        {
            throw std::runtime_error("[Pipeline] Null drawable collection");
        }

        if (drawableCollection->getAttributeDescriptions().size() == 0)
        {
            throw std::runtime_error("[Pipeline] Empty drawable collection");
        }

        this->lDevice = lDevice;
        this->collection = std::move(drawableCollection);

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

        // In case of a depth buffer
        VkPipelineDepthStencilStateCreateInfo depthStencil{};

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
            shaderStages.push_back(createInfo);
        }

        // Add the dynamic state (TODO make this configurable)
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState{};

        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Vertex input buffer (TODO configure this to have a mutable vertex buffer)
        VkPipelineVertexInputStateCreateInfo vertexInfo{};

        VkVertexInputBindingDescription bindingDescription = collection->getBindingDescription();
        std::vector<VkVertexInputAttributeDescription> attributeDescription = collection->getAttributeDescriptions();

        vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInfo.vertexBindingDescriptionCount = 1;
        vertexInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
        vertexInfo.pVertexAttributeDescriptions = attributeDescription.data();

        // Input assembly (TODO make this configurable)
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};

        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport and scissors
        VkPipelineViewportStateCreateInfo viewportState{};

        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Rasterizer (TODO make this configurable)
        VkPipelineRasterizationStateCreateInfo rasterizer{};

        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = config.polygonMode;
        rasterizer.cullMode = config.cullMode;
        rasterizer.frontFace = config.frontFace;
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
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};

        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};

        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        // Pipeline layout (aka Uniforms TODO make this configurable)
        VkPipelineLayoutCreateInfo layoutInfo{};

        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 0;
        layoutInfo.pSetLayouts = nullptr;
        layoutInfo.pushConstantRangeCount = 0;
        layoutInfo.pPushConstantRanges = nullptr;

        if (collection->hasDescriptorSet())
        {
            layoutInfo.setLayoutCount = 1;
            layoutInfo.pSetLayouts = &collection->getDescriptorSetLayout();
        }

        if (vkCreatePipelineLayout(lDevice->getDevice(), &layoutInfo, nullptr, &layout) != VK_SUCCESS)
        {
            throw std::runtime_error("[Pipeline] Error creating pipeline layout");
        }

        // Create the pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};

        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(collection->getShaders().size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = layout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        // Check if depth testing is enabled
        if (depthTestType != NONE)
        {
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.stencilTestEnable = VK_FALSE;

            pipelineInfo.pDepthStencilState = &depthStencil;
        }

        if (vkCreateGraphicsPipelines(lDevice->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("[Pipeline] Error creating the pipeline");
        }
    }

    Pipeline::~Pipeline()
    {
        if (layout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(lDevice->getDevice(), layout, nullptr);
        }

        if (pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(lDevice->getDevice(), pipeline, nullptr);
        }
    }
}