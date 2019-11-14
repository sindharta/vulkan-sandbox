#pragma once

#include <vulkan/vulkan.h> 
#include <stdint.h>
#include <vector>

#include "DrawObject.h"

namespace Shin {

class DrawPipeline {
public:

    DrawPipeline();
    void Init(const VkDevice device, VkAllocationCallbacks* allocator,
        const char* vsPath, const char* fsPath,
        const VkVertexInputBindingDescription*  bindingDescriptions,
        const std::vector<VkVertexInputAttributeDescription>* attributeDescriptions,
        const VkDescriptorSetLayout descriptorSetLayout
    );

    //RenderPass is created when swap chain is changed (swapChainSurfaceFormat might have changed)
    void RecreateSwapChainObjects( const VkPhysicalDevice physicalDevice, const VkDevice device, 
        VkAllocationCallbacks* allocator, VkDescriptorPool descriptorPool, const uint32_t numImages,
        const VkRenderPass renderPass,
        const VkExtent2D& extent
    );

    void CleanUpSwapChainObjects(const VkDevice device, VkAllocationCallbacks* allocator);
    void CleanUp(const VkDevice device, VkAllocationCallbacks* allocator);

    void Bind(const VkCommandBuffer commandBuffer);

    void DrawToCommandBuffer(const VkCommandBuffer commandBuffer, const uint32_t imageIndex);
    void AddDrawObject(DrawObject* obj);
private:

    std::vector<DrawObject*>     m_drawObjects; // multiple objects

    VkPipeline                  m_pipeline;
    VkPipelineLayout            m_pipelineLayout; //to pass uniform values to shaders

    //[TODO-sin: 2019-11-13] Can these five be grouped as something ?
    VkShaderModule                                          m_vertShaderModule;
    VkShaderModule                                          m_fragShaderModule;
    const VkVertexInputBindingDescription*                  m_bindingDescriptions;
    const std::vector<VkVertexInputAttributeDescription>*   m_attributeDescriptions;
    VkDescriptorSetLayout                                   m_descriptorSetLayout;



};

};
