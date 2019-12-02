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

        static VkResult CopyBuffer(const VkDevice device, const VkCommandPool commandPool, const VkQueue queue, 
                               const VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
                               

        static VkImageView  CreateImageView(const VkDevice device, const VkAllocationCallbacks* allocator, 
                                            const VkImage image, const VkFormat format);

        static VkDeviceSize CreateImage(const VkPhysicalDevice physicalDevice, const VkDevice device, 
                                const VkAllocationCallbacks* allocator,
                                const uint32_t width, const uint32_t height, 
                                const VkImageTiling tiling, const VkImageUsageFlags usage,
                                const VkMemoryPropertyFlags properties, const VkFormat format,
                                VkImage* image, VkDeviceMemory* imageMemory, bool exportHandle = false);

        static VkResult DoImageLayoutTransition(const VkDevice device, const VkCommandPool commandPool, const VkQueue queue, 
                                          VkImage image, VkFormat format, 
                                          VkImageLayout oldLayout, VkImageLayout newLayout); 
        
        static VkResult CopyBufferToImage(const VkDevice device, const VkCommandPool commandPool, const VkQueue queue, 
                               const VkBuffer buffer, VkImage image, const uint32_t width, const uint32_t height);

        static VkResult BeginOneTimeCommandBufferInto(const VkDevice device, const VkCommandPool commandPool, 
            VkCommandBuffer* commandBuffer);

        //Uses vkQueueWaitIdle to synchronize
        static VkResult EndAndSubmitOneTimeCommandBuffer(const VkDevice device, const VkCommandPool commandPool, 
                                                      const VkQueue queue, VkCommandBuffer commandBuffer);

        //The destMemory must have been created using 
        //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT properties
        static void CopyCPUDataToBuffer(const VkDevice device, const void* src, const VkDeviceMemory destMemory, 
                               const VkDeviceSize size);


        static void GetPhysicalDeviceUUIDInto(
            VkInstance instance, VkPhysicalDevice phyDevice,
            std::array<uint8_t, VK_UUID_SIZE>* deviceUUID
        );

        static void* GetExportHandle(const VkDevice device, const VkDeviceMemory memory);


};
    
