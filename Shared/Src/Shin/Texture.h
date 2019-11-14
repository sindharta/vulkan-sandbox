#pragma once

#include <vulkan/vulkan.h> 

namespace Shin {
class Texture {

public:
    Texture();
    void Init(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        const VkAllocationCallbacks* allocator,  const VkCommandPool commandPool, VkQueue queue, const char* path);

    void InitAsRenderTexture(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        const VkAllocationCallbacks* allocator, const uint32_t width, const uint32_t height);

    void CleanUp(const VkDevice device, const VkAllocationCallbacks* allocator);

    inline VkImageView GetImageView() const;
    inline VkSampler GetSampler() const;
private:

    void CreateTextureImage(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        const VkAllocationCallbacks* allocator,  const VkCommandPool commandPool, VkQueue queue, const char* path);
    void CreateTextureImageView(const VkDevice device, const VkAllocationCallbacks* allocator);
    void CreateTextureSampler(const VkDevice device, const VkAllocationCallbacks* allocator);

    VkImage             m_textureImage;
    VkDeviceMemory      m_textureImageMemory;
    VkImageView         m_textureImageView;
    VkSampler           m_textureSampler;
};

//---------------------------------------------------------------------------------------------------------------------

VkImageView Texture::GetImageView() const { return m_textureImageView; }
VkSampler Texture::GetSampler() const { return m_textureSampler; }

} //end namespace