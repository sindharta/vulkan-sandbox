#pragma once
#include <vulkan/vulkan.h> 
#include <vector>

class GraphicsUtility {
    public:
        static VkShaderModule CreateShaderModule(VkDevice* logicalDevice, const VkAllocationCallbacks* allocator, 
                                                 const std::vector<char>& code);
};