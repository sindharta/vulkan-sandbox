#include "ShaderUtility.h"

VkShaderModule ShaderUtility::CreateShaderModule(VkDevice* logicalDevice, const VkAllocationCallbacks* allocator, 
                                                 const std::vector<char>& code) {

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(*logicalDevice, &createInfo, allocator, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
   
}
