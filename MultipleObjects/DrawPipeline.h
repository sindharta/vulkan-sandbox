#pragma once

#include <vulkan/vulkan.h> 
#include <stdint.h>
#include <vector>

#include "DrawObject.h"

class DrawPipeline {
public:

    DrawPipeline();
    void Init(const VkDevice device, VkAllocationCallbacks* allocator,
        const char* vsPath, const char* fsPath,
        const VkVertexInputBindingDescription*  bindingDescriptions,
        const std::vector<VkVertexInputAttributeDescription>* attributeDescriptions,
        const VkDescriptorSetLayout* descriptorSetLayout
    );

    //RenderPass is created when swap chain is changed (swapChainSurfaceFormat might have changed)
    void RecreateSwapChainObjects( const VkDevice device, VkAllocationCallbacks* allocator,         
        const VkRenderPass renderPass,
        const VkExtent2D& extent
    );

    void CleanUpSwapChainObjects(const VkDevice device, VkAllocationCallbacks* allocator);
    void CleanUp(const VkDevice device, VkAllocationCallbacks* allocator);

    void DrawToCommandBuffer(const VkCommandBuffer commandBuffer, const uint32_t imageIndex);
    void AddDrawObject(const DrawObject* obj);
    inline const VkPipeline GetPipeline() const;
private:

    std::vector<const DrawObject*>     m_drawObjects; // multiple objects

    VkPipeline                  m_pipeline;
    VkPipelineLayout            m_pipelineLayout; //to pass uniform values to shaders

    //[TODO-sin: 2019-11-13] Can these five be grouped as something ?
    VkShaderModule                                          m_vertShaderModule;
    VkShaderModule                                          m_fragShaderModule;
    const VkVertexInputBindingDescription*                  m_bindingDescriptions;
    const std::vector<VkVertexInputAttributeDescription>*   m_attributeDescriptions;
    const VkDescriptorSetLayout*                            m_descriptorSetLayout;



};