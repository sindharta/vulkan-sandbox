#include "GraphicsUtility.h"

VkShaderModule GraphicsUtility::CreateShaderModule(const VkDevice device, const VkAllocationCallbacks* allocator, 
                                                 const std::vector<char>& code) {

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, allocator, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
   
}

//---------------------------------------------------------------------------------------------------------------------

uint32_t GraphicsUtility::FindMemoryType(const VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {

        //properties define special features of the memory, like being able to map so we can write to it from the CPU. 
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");

}

//---------------------------------------------------------------------------------------------------------------------

void GraphicsUtility::CreateBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, 
                                   const VkAllocationCallbacks* allocator,
                                   const VkDeviceSize size, 
                                   const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, 
                                   VkBuffer* buffer, VkDeviceMemory* bufferMemory) 
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    //Get the memory requirement of the VB
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

//---------------------------------------------------------------------------------------------------------------------

void GraphicsUtility::CopyBuffer(const VkDevice device, const VkCommandPool commandPool, const VkQueue queue,
                                 const VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) 
{
    VkCommandBuffer commandBuffer = BeginOneTimeCommandBuffer(device,commandPool);

    //Start copy
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    EndAndSubmitOneTimeCommandBuffer(device,commandPool,queue,commandBuffer);
}

//---------------------------------------------------------------------------------------------------------------------
VkCommandBuffer GraphicsUtility::BeginOneTimeCommandBuffer(const VkDevice device, const VkCommandPool commandPool)   {
    //Create a command buffer to copy
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //used only once
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

//---------------------------------------------------------------------------------------------------------------------
void GraphicsUtility::EndAndSubmitOneTimeCommandBuffer(const VkDevice device, const VkCommandPool commandPool, 
                                                       VkQueue queue, VkCommandBuffer commandBuffer) 
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

}

//---------------------------------------------------------------------------------------------------------------------

void GraphicsUtility::CreateImage(const VkPhysicalDevice physicalDevice, const VkDevice device, 
    const VkAllocationCallbacks* allocator,
    const uint32_t width, const uint32_t height,
    VkImageTiling tiling, VkImageUsageFlags usage,VkMemoryPropertyFlags properties,
    VkImage* image, VkDeviceMemory* imageMemory) 
{

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional
    if (vkCreateImage(device, &imageInfo, allocator, image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, *image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = GraphicsUtility::FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, allocator, imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, *image, *imageMemory, 0);
}


