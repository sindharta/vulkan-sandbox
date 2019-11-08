#pragma once
#include <vulkan/vulkan.h> 
#include <vector>

class GraphicsUtility {
    public:
        static VkShaderModule CreateShaderModule(const VkDevice device, const VkAllocationCallbacks* allocator, 
                                                 const std::vector<char>& code);

        static uint32_t FindMemoryType(const VkPhysicalDevice physicalDevice, uint32_t typeFilter, 
                                       VkMemoryPropertyFlags properties);

        static void CreateBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, 
                                 const VkAllocationCallbacks* allocator,
                                 const VkDeviceSize size, 
                                 const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, 
                                 VkBuffer* buffer, VkDeviceMemory* bufferMemory);

        static void CopyBuffer(const VkDevice device, const VkCommandPool commandPool, const VkQueue queue, 
                               const VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        static void CreateImage(const VkPhysicalDevice physicalDevice, const VkDevice device, 
                                const VkAllocationCallbacks* allocator,
                                const uint32_t width, const uint32_t height, 
                                VkImageTiling tiling, VkImageUsageFlags usage,
                                VkMemoryPropertyFlags properties, 
                                VkImage* image, VkDeviceMemory* imageMemory);

        static VkCommandBuffer BeginOneTimeCommandBuffer(const VkDevice device, const VkCommandPool commandPool);
        static void  EndAndSubmitOneTimeCommandBuffer(const VkDevice device, const VkCommandPool commandPool, 
                                                      VkQueue queue, VkCommandBuffer commandBuffer);

};
    
