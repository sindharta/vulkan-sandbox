#include "DrawPipeline.h"

#include "Utilities/GraphicsUtility.h"
#include "Utilities/FileUtility.h"
#include "Utilities/Macros.h"

#include "Mesh.h"
#include "DrawObject.h"

DrawPipeline::DrawPipeline() : m_pipeline(VK_NULL_HANDLE), m_pipelineLayout(VK_NULL_HANDLE), 
    m_bindingDescriptions(nullptr), m_attributeDescriptions(nullptr),
    m_descriptorSetLayout(nullptr)
{

}

//---------------------------------------------------------------------------------------------------------------------
void DrawPipeline::Init( const VkDevice device, VkAllocationCallbacks* allocator, 
    const char* vsPath, const char* fsPath,
    const VkVertexInputBindingDescription*  bindingDescriptions,
    const std::vector<VkVertexInputAttributeDescription>* attributeDescriptions,
    const VkDescriptorSetLayout descriptorSetLayout
) 
{
    std::vector<char> vertShaderCode, fragShaderCode;
    FileUtility::ReadFileInto(vsPath, &vertShaderCode);
    FileUtility::ReadFileInto(fsPath, &fragShaderCode);

    m_vertShaderModule = GraphicsUtility::CreateShaderModule(device, allocator, vertShaderCode);
    m_fragShaderModule = GraphicsUtility::CreateShaderModule(device, allocator, fragShaderCode);

    m_bindingDescriptions = bindingDescriptions;
    m_attributeDescriptions = attributeDescriptions;
    m_descriptorSetLayout = descriptorSetLayout;
}

//---------------------------------------------------------------------------------------------------------------------
void DrawPipeline::CleanUpSwapChainObjects(const VkDevice device, VkAllocationCallbacks* allocator) {
    SAFE_DESTROY_PIPELINE(device, m_pipeline, allocator);
    SAFE_DESTROY_PIPELINE_LAYOUT(device, m_pipelineLayout, allocator);
}

//---------------------------------------------------------------------------------------------------------------------

void DrawPipeline::CleanUp(const VkDevice device, VkAllocationCallbacks* allocator) {
    m_drawObjects.clear();

    SAFE_DESTROY_SHADER_MODULE(device, m_fragShaderModule, allocator);
    SAFE_DESTROY_SHADER_MODULE(device, m_vertShaderModule, allocator);

}

//---------------------------------------------------------------------------------------------------------------------

void DrawPipeline::RecreateSwapChainObjects( const VkPhysicalDevice physicalDevice, const VkDevice device, 
        VkAllocationCallbacks* allocator, VkDescriptorPool descriptorPool, const uint32_t numImages,
        const VkRenderPass renderPass,
        const VkExtent2D& extent
    )
{
    //Vertex
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = m_vertShaderModule;
    vertShaderStageInfo.pName = "main";
    vertShaderStageInfo.pSpecializationInfo = nullptr;//To specify shader constants

    //Frag
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = m_fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    //Vertex Input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attributeDescriptions->size());
    vertexInputInfo.pVertexBindingDescriptions = m_bindingDescriptions; 
    vertexInputInfo.pVertexAttributeDescriptions = m_attributeDescriptions->data(); 

    //Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    //Viewport and Scissor 
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    //Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    //Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    //Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    //Dynamic states
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    //Pipeline layout: to pass uniform values to shaders
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; 
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout; 
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, allocator, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    //Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, allocator, &m_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }


    //Registered draw objects
    const uint32_t numDrawObjects = static_cast<uint32_t>(m_drawObjects.size());
    for (uint32_t i=0;i<numDrawObjects;++i) {
        m_drawObjects[i]->RecreateSwapChainObjects(physicalDevice, device, allocator, 
            descriptorPool, numImages, m_descriptorSetLayout);
        m_drawObjects[i]->SetProj(extent.width / static_cast<float> (extent.height));
    }

}

//---------------------------------------------------------------------------------------------------------------------

void DrawPipeline::DrawToCommandBuffer(const VkCommandBuffer commandBuffer, const uint32_t imageIndex) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    //Draw multiple objects
    const uint32_t numObjects = static_cast<uint32_t>(m_drawObjects.size());
    for (uint32_t k = 0; k < numObjects; ++k) {
        const DrawObject* curDrawObject = m_drawObjects[k];
        const Mesh* curMesh = curDrawObject->GetMesh();

        //Bind vertex and index buffers
        VkBuffer vertexBuffers[] = { curMesh->GetVertexBuffer() };
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, curMesh->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);

        const VkDescriptorSet& curDescriptorSet = curDrawObject->GetDescriptorSet(imageIndex);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
            m_pipelineLayout, 0, 1, &curDescriptorSet, 0, nullptr
        );

        vkCmdDrawIndexed(commandBuffer, curMesh->GetNumIndices(), 1, 0, 0, 0);
    }

}

//---------------------------------------------------------------------------------------------------------------------

void DrawPipeline::AddDrawObject(DrawObject* obj) {
    m_drawObjects.push_back(obj);
}
