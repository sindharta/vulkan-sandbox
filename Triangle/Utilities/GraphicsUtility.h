#pragma once
#include <vulkan/vulkan.h> 
#include <vector>

class GraphicsUtility {
    public:
        static VkShaderModule CreateShaderModule(const VkDevice device, const VkAllocationCallbacks* allocator, 
                                                 const std::vector<char>& code);

        static uint32_t FindMemoryType(const VkPhysicalDevice physicalDevice, uint32_t typeFilter, 
                                       VkMemoryPropertyFlags properties);

        static void CreateBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, VkDeviceSize size, 
                                 VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                                 VkBuffer* buffer, VkDeviceMemory* bufferMemory);
    
};