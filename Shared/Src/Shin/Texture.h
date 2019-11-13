#pragma once

#include <vulkan/vulkan.h> 

namespace Shin {
class Texture {

public:
    Texture();
    void Init(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        VkAllocationCallbacks* allocator,  const VkCommandPool commandPool, VkQueue queue, const char* path);
    void CleanUp(const VkDevice device,VkAllocationCallbacks* allocator);

    inline VkImageView GetImageView() const;
    inline VkSampler GetSampler() const;
private:

    void CreateTextureImage(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        VkAllocationCallbacks* allocator,  const VkCommandPool commandPool, VkQueue queue, const char* path);
    void CreateTextureImageView(const VkDevice device, VkAllocationCallbacks* allocator);
    void CreateTextureSampler(const VkDevice device, VkAllocationCallbacks* allocator);

    VkImage             m_textureImage;
    VkDeviceMemory      m_textureImageMemory;
    VkImageView         m_textureImageView;
    VkSampler           m_textureSampler;
};

//---------------------------------------------------------------------------------------------------------------------

VkImageView Texture::GetImageView() const { return m_textureImageView; }
VkSampler Texture::GetSampler() const { return m_textureSampler; }

} //end namespace