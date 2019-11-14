#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h> 

// Framebuffer for offscreen rendering
struct FrameBufferAttachment {
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
};

namespace Shin {


class OffScreenPass {

public:
    void Init(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        const VkAllocationCallbacks* allocator, const uint32_t width, const uint32_t height);
    void CleanUp(const VkDevice device, VkAllocationCallbacks* allocator);

private:

	uint32_t m_width, m_height;
	FrameBufferAttachment m_color;

	VkFramebuffer m_frameBuffer;		
	VkRenderPass m_renderPass;
	VkSampler m_sampler;
	VkDescriptorImageInfo m_descriptor;

} ;

} //end namespace
