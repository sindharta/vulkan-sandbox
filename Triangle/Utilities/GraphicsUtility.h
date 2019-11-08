#pragma once
#include <vulkan/vulkan.h> 
#include <vector>

//[TODO-sin: 2019-11-8] No need to pass pointers.
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

        static VkImageView  CreateImageView(const VkDevice device, const VkAllocationCallbacks* allocator, 
                                            const VkImage image, const VkFormat format);

        static void CreateImage(const VkPhysicalDevice physicalDevice, const VkDevice device, 
                                const VkAllocationCallbacks* allocator,
                                const uint32_t width, const uint32_t height, 
                                const VkImageTiling tiling, const VkImageUsageFlags usage,
                                const VkMemoryPropertyFlags properties, 
                                VkImage* image, VkDeviceMemory* imageMemory);

        static void DoImageLayoutTransition(const VkDevice device, const VkCommandPool commandPool, const VkQueue queue, 
                                          VkImage* image, VkFormat format, 
                                          VkImageLayout oldLayout, VkImageLayout newLayout); 
        
        static void CopyBufferToImage(const VkDevice device, const VkCommandPool commandPool, const VkQueue queue, 
                               const VkBuffer buffer, VkImage image, const uint32_t width, const uint32_t height);

        static VkCommandBuffer BeginOneTimeCommandBuffer(const VkDevice device, const VkCommandPool commandPool);

        //Uses vkQueueWaitIdle to synchronize
        static void  EndAndSubmitOneTimeCommandBuffer(const VkDevice device, const VkCommandPool commandPool, 
                                                      const VkQueue queue, VkCommandBuffer commandBuffer);



};
    
