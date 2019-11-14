#include "OffScreenPass.h"
#include <array>

#include "Shin/Utilities/GraphicsUtility.h"
#include "Shin/Utilities/Macros.h"

namespace Shin {

OffScreenPass::OffScreenPass() : m_renderPass(VK_NULL_HANDLE)
{

}

//---------------------------------------------------------------------------------------------------------------------

void OffScreenPass::Init(const uint32_t width, const uint32_t height) {
    m_extent.width  = width;
    m_extent.height = height;
}

//---------------------------------------------------------------------------------------------------------------------

void OffScreenPass::CleanUp(const VkDevice device, const VkAllocationCallbacks* allocator) {

	SAFE_DESTROY_RENDER_PASS(device, m_renderPass, allocator);

    const uint32_t numImages = static_cast<uint32_t>(m_colors.size());
    for (uint32_t i=0;i<numImages;++i) {
        CleanUpSwapChainObject(device, allocator, i);
    }
    m_colors.clear();
	m_frameBuffers.clear();		
}

//---------------------------------------------------------------------------------------------------------------------

void OffScreenPass::RecreateSwapChainObjects(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        const VkAllocationCallbacks* allocator, const uint32_t numImages) 
{
    //Check RenderPass
    if (VK_NULL_HANDLE == m_renderPass) {
        CreateRenderPass(device, allocator);
    }

    const uint32_t prevNumImages = static_cast<uint32_t>(m_colors.size());
    if (prevNumImages == numImages) {
        return;
    }

    //if we currently have too many
    if (prevNumImages > numImages) {
        for (uint32_t i = numImages; i < prevNumImages; ++i) {
            CleanUpSwapChainObject(device, allocator, i);
        }
        m_colors.resize(numImages);
	    m_frameBuffers.resize(numImages);		
        return;
    }

    //we need more 
    m_colors.resize(numImages);
	m_frameBuffers.resize(numImages);		
    for (uint32_t i = prevNumImages; i < numImages; ++i) {
        CreateSwapChainObject(physicalDevice, device, allocator, i);
    }
    

}

//---------------------------------------------------------------------------------------------------------------------

void OffScreenPass::CreateSwapChainObject(const VkPhysicalDevice physicalDevice, const VkDevice device,
    const VkAllocationCallbacks* allocator, const uint32_t imageIndex) 
{
    Texture* curColor               = &m_colors[imageIndex];
    VkFramebuffer* curFrameBuffer   = &m_frameBuffers[imageIndex];

	// Create image and image view
    curColor->InitAsRenderTexture(physicalDevice, device, allocator, m_extent.width, m_extent.height);

    //Create Frame Buffer
	VkImageView attachments[1];
	attachments[0] = curColor->GetImageView();

	VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = m_renderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = attachments;
	framebufferInfo.width = m_extent.width;
	framebufferInfo.height = m_extent.height;
	framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, allocator, curFrameBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Offscreen framebuffer!");
    }

	// Fill a descriptor for later use in a descriptor set 
	m_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_descriptor.imageView = curColor->GetImageView();
	m_descriptor.sampler = curColor->GetSampler();
}

//---------------------------------------------------------------------------------------------------------------------
void OffScreenPass::CreateRenderPass(const VkDevice device, const VkAllocationCallbacks* allocator) 
{
	// Create a separate render pass for the offscreen rendering 
	std::array<VkAttachmentDescription, 1> colorAttachment = {};

	// Color attachment
	colorAttachment[0].format = VK_FORMAT_R8G8B8A8_UNORM;
	colorAttachment[0].samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

    //[Note-sin: 2019-11-14] All reads in the fragment shader must be finished before we start writing
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //[Note-sin: 2019-11-14] All writes must be finished before we start reading in fragment shader
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create the actual renderpass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(colorAttachment.size());
	renderPassInfo.pAttachments = colorAttachment.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &renderPassInfo, allocator, &m_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

}

//---------------------------------------------------------------------------------------------------------------------

void OffScreenPass::CleanUpSwapChainObject(const VkDevice device, const VkAllocationCallbacks* allocator, 
                                           const uint32_t imageIndex)     
{
    m_colors[imageIndex].CleanUp(device, allocator);
	vkDestroyFramebuffer(device, m_frameBuffers[imageIndex], allocator);
}


} //end namespace
