#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h> 
#include <vector>
#include "Shin/Texture.h"


namespace Shin {

class OffScreenPass {

public:
    OffScreenPass();
    void Init(const uint32_t width, const uint32_t height);
    void CleanUp(const VkDevice device, const VkAllocationCallbacks* allocator);

    //Swap chain
    void RecreateSwapChainObjects(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        const VkAllocationCallbacks* allocator, const uint32_t numImages);

    inline const Texture* GetTexture(const uint32_t idx) const;
	inline VkFramebuffer GetFrameBuffer(const uint32_t idx) const;		
    inline VkRenderPass GetRenderPass() const;
    inline VkExtent2D GetExtent() const;

private:

    void CreateSwapChainObject(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        const VkAllocationCallbacks* allocator, const uint32_t imageIndex);
    void CleanUpSwapChainObject(const VkDevice device, const VkAllocationCallbacks* allocator, const uint32_t imageIndex);

    void CreateRenderPass(const VkDevice device, const VkAllocationCallbacks* allocator);

    VkExtent2D m_extent;

    //What should be allocated as many as swap chain images
    std::vector<Texture> m_colors;
	std::vector<VkFramebuffer> m_frameBuffers;		
	VkRenderPass  m_renderPass;

	VkDescriptorImageInfo m_descriptor;


} ;

//---------------------------------------------------------------------------------------------------------------------

const Texture* OffScreenPass::GetTexture(const uint32_t idx) const { return &m_colors[idx];}
VkFramebuffer OffScreenPass::GetFrameBuffer(const uint32_t idx) const { return m_frameBuffers[idx]; }		
VkRenderPass OffScreenPass::GetRenderPass() const { return m_renderPass; }
VkExtent2D OffScreenPass::GetExtent() const { return m_extent; }

} //end namespace
