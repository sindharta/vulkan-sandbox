#include "GraphicsUtility.h"
#include <array>
#include "Macros.h"

#ifdef _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#ifndef _WIN32
#define EXTERNAL_MEMORY_HANDLE_SUPPORTED_TYPE    VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR
#else
#define EXTERNAL_MEMORY_HANDLE_SUPPORTED_TYPE    VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR
#endif

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

VkResult GraphicsUtility::CopyBuffer(const VkDevice device, const VkCommandPool commandPool, const VkQueue queue,
                                 const VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) 
{
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VULKAN_CHECK(BeginOneTimeCommandBufferInto(device, commandPool, &commandBuffer));

    //Start copy
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    return EndAndSubmitOneTimeCommandBuffer(device,commandPool,queue,commandBuffer);
}

//---------------------------------------------------------------------------------------------------------------------
VkImageView  GraphicsUtility::CreateImageView(const VkDevice device, const VkAllocationCallbacks* allocator,
                                              const VkImage image, const VkFormat format)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;


    VkImageView imageView = VK_NULL_HANDLE;
    if (vkCreateImageView(device, &viewInfo, allocator, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

//---------------------------------------------------------------------------------------------------------------------

//initialLayout must be either VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED
//We use VK_IMAGE_LAYOUT_UNDEFINED here.
VkDeviceSize GraphicsUtility::CreateImage(const VkPhysicalDevice physicalDevice, const VkDevice device, 
    const VkAllocationCallbacks* allocator,
    const uint32_t width, const uint32_t height,
    const VkImageTiling tiling, const VkImageUsageFlags usage, const VkMemoryPropertyFlags properties,
    const VkFormat format,
    VkImage* image, VkDeviceMemory* imageMemory, bool exportHandle) 
{

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
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

    VkExportMemoryAllocateInfoKHR exportInfo = {};
    if (exportHandle)  {
        exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
        exportInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
        allocInfo.pNext = &exportInfo;
    }

    if (vkAllocateMemory(device, &allocInfo, allocator, imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, *image, *imageMemory, 0);

    return memRequirements.size;
}

//---------------------------------------------------------------------------------------------------------------------

VkResult GraphicsUtility::DoImageLayoutTransition(const VkDevice device, const VkCommandPool commandPool, const VkQueue queue, 
                                          VkImage image, VkFormat format, 
                                          VkImageLayout oldLayout, VkImageLayout newLayout) 
{ 
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VULKAN_CHECK(BeginOneTimeCommandBufferInto(device, commandPool, &commandBuffer));

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; //for transferring queue family ownership
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1; //No mip map
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = 0;
    VkPipelineStageFlags destinationStage = 0;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        //Undefined -> transfer destination: transfer writes that don't need to wait on anything
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; //the earliest possible pipeline stage
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        //Transfer destination -> shader reading: 
        //shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, 
        //because that's where we're going to use the texture
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    return EndAndSubmitOneTimeCommandBuffer(device, commandPool, queue, commandBuffer);
}

//---------------------------------------------------------------------------------------------------------------------

VkResult GraphicsUtility::CopyBufferToImage(const VkDevice device, const VkCommandPool commandPool, const VkQueue queue,
    const VkBuffer buffer, VkImage image, const uint32_t width, const uint32_t height)         
{
    VkCommandBuffer commandBuffer;
    VULKAN_CHECK(BeginOneTimeCommandBufferInto(device, commandPool, &commandBuffer));

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    return EndAndSubmitOneTimeCommandBuffer(device, commandPool, queue, commandBuffer);
}

//---------------------------------------------------------------------------------------------------------------------
VkResult GraphicsUtility::BeginOneTimeCommandBufferInto(const VkDevice device, const VkCommandPool commandPool, 
    VkCommandBuffer* commandBuffer)
{
    //Create a command buffer to copy
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(device, &allocInfo, commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //used only once
    VULKAN_CHECK(vkBeginCommandBuffer(*commandBuffer, &beginInfo));

    return VK_SUCCESS;

}

//---------------------------------------------------------------------------------------------------------------------
VkResult GraphicsUtility::EndAndSubmitOneTimeCommandBuffer(const VkDevice device, const VkCommandPool commandPool, 
                                                       VkQueue queue, VkCommandBuffer commandBuffer) 
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VULKAN_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    VULKAN_CHECK(vkQueueWaitIdle(queue));

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    return VK_SUCCESS;
}

//---------------------------------------------------------------------------------------------------------------------
void GraphicsUtility::CopyCPUDataToBuffer(const VkDevice device, const void* src, const VkDeviceMemory destMemory,
    const VkDeviceSize size) 
{
    void* data = nullptr;
    vkMapMemory(device, destMemory, 0, size, 0, &data);
    memcpy(data, src, size);
    vkUnmapMemory(device, destMemory);

}

//---------------------------------------------------------------------------------------------------------------------
void GraphicsUtility::GetPhysicalDeviceUUIDInto(VkInstance instance, VkPhysicalDevice phyDevice, std::array<uint8_t, VK_UUID_SIZE>* deviceUUID) 
{
    /*
     * Query the physical device properties to obtain the device UUID.
     * Note that successfully loading vkGetPhysicalDeviceProperties2KHR()
     * requires the VK_KHR_get_physical_device_properties2 extension
     * (which is an instance-level extension) to be enabled.
     */
    VkPhysicalDeviceIDPropertiesKHR deviceIDProps = {};
    deviceIDProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR;

    VkPhysicalDeviceProperties2KHR props = {};
    props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
    props.pNext = &deviceIDProps;

    auto func = (PFN_vkGetPhysicalDeviceProperties2KHR) \
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR");
    if (func == nullptr) {
        throw std::runtime_error("Failed to load vkGetPhysicalDeviceProperties2KHR");
    }

    func(phyDevice, &props);

    std::memcpy(deviceUUID->data(), deviceIDProps.deviceUUID, VK_UUID_SIZE);

}

//---------------------------------------------------------------------------------------------------------------------

#ifndef _WIN32
void* GraphicsUtility::GetExportHandle(const VkDevice device, const VkDeviceMemory memory)
{
    int fd = -1;

    VkMemoryGetFdInfoKHR fdInfo = {};
    fdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    fdInfo.memory = memory;
    fdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;

    auto func = (PFN_vkGetMemoryFdKHR) \
        vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");

    if (!func ||
        func(device, &fdInfo, &fd) != VK_SUCCESS) {
        return nullptr;
    }

    return (void *)(uintptr_t)fd;
}
#else
void* GraphicsUtility::GetExportHandle(const VkDevice device, const VkDeviceMemory memory)
{
    HANDLE handle = nullptr;

    VkMemoryGetWin32HandleInfoKHR handleInfo = {};
    handleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    handleInfo.memory = memory;
    handleInfo.handleType = EXTERNAL_MEMORY_HANDLE_SUPPORTED_TYPE;

    auto func = (PFN_vkGetMemoryWin32HandleKHR) \
        vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR");

    if (!func ||
        func(device, &handleInfo, &handle) != VK_SUCCESS) {
        return nullptr;
    }

    return (void *)handle;
}
#endif

