#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION    //This will include the implementation of STB, instead of only the header
#include "stb_image.h"  //for loading images
#include <stdexcept> //std::runtime_error

#include "Utilities/Macros.h"
#include "Utilities/GraphicsUtility.h"

namespace Shin {

Texture::Texture() : m_textureImage(VK_NULL_HANDLE), m_textureImageMemory(VK_NULL_HANDLE)
    , m_textureImageView(VK_NULL_HANDLE), m_textureSampler(VK_NULL_HANDLE)
{

}

//---------------------------------------------------------------------------------------------------------------------

void Texture::Init(const VkPhysicalDevice physicalDevice, const VkDevice device, VkAllocationCallbacks* allocator,  
    const VkCommandPool commandPool, VkQueue queue, const char* path)
{
    CreateTextureImage(physicalDevice, device, allocator, commandPool, queue, path);
    CreateTextureImageView(device, allocator);
    CreateTextureSampler(device, allocator);
}

//---------------------------------------------------------------------------------------------------------------------
void Texture::CreateTextureImage(const VkPhysicalDevice physicalDevice, const VkDevice device, 
    VkAllocationCallbacks* allocator,  const VkCommandPool commandPool, VkQueue queue, const char* path) 
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    const VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    GraphicsUtility::CreateBuffer(physicalDevice, device, allocator, imageSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        &stagingBuffer, &stagingBufferMemory
    );

    GraphicsUtility::CopyCPUDataToBuffer(device,pixels,stagingBufferMemory,imageSize);

    stbi_image_free(pixels);

    //Create Image buffer. 
    //VK_IMAGE_USAGE_SAMPLED_BIT: to allow access from the shader
    GraphicsUtility::CreateImage(physicalDevice,device,allocator, texWidth, texHeight,
        VK_IMAGE_TILING_OPTIMAL,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,VK_FORMAT_R8G8B8A8_UNORM, &m_textureImage,&m_textureImageMemory
    );

    //Transition the texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    GraphicsUtility::DoImageLayoutTransition(device, commandPool, queue, 
        m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, 
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    //Execute the buffer to image copy operation
    GraphicsUtility::CopyBufferToImage(device, commandPool, queue,stagingBuffer, 
        m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    //one last transition to prepare it for shader access:
    GraphicsUtility::DoImageLayoutTransition(device, commandPool, queue, 
        m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    vkDestroyBuffer(device, stagingBuffer, allocator);
    vkFreeMemory(device, stagingBufferMemory, allocator);

}

//---------------------------------------------------------------------------------------------------------------------

void Texture::CreateTextureImageView(const VkDevice device, VkAllocationCallbacks* allocator) { 
    m_textureImageView = GraphicsUtility::CreateImageView(device, 
        allocator, m_textureImage, VK_FORMAT_R8G8B8A8_UNORM);

}

//---------------------------------------------------------------------------------------------------------------------
void Texture::CreateTextureSampler(const VkDevice device, VkAllocationCallbacks* allocator) {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE; //[0..1] range
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    if (vkCreateSampler(device, &samplerInfo, allocator, &m_textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

//---------------------------------------------------------------------------------------------------------------------

void Texture::CleanUp(const VkDevice device, VkAllocationCallbacks* allocator) {

    SAFE_DESTROY_SAMPLER(device,m_textureSampler,allocator);
    SAFE_DESTROY_IMAGE_VIEW(device, m_textureImageView, allocator);
    SAFE_DESTROY_IMAGE(device, m_textureImage, allocator);
    SAFE_FREE_MEMORY(device, m_textureImageMemory, allocator);
}

} //end namespace
