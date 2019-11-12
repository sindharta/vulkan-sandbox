
#include "Mesh.h"
#include "Utilities/GraphicsUtility.h"
#include "Utilities/Macros.h"


Mesh::Mesh() : m_vb(VK_NULL_HANDLE), m_vbMemory(VK_NULL_HANDLE), 
                         m_ib(VK_NULL_HANDLE), m_ibMemory(VK_NULL_HANDLE)
{

}

//---------------------------------------------------------------------------------------------------------------------

void Mesh::Init(const VkPhysicalDevice physicalDevice, const VkDevice device, 
    VkAllocationCallbacks* allocator,  const VkCommandPool commandPool, VkQueue queue, 
    const char* vertexData, const uint32_t vertexDataSize, const char* indexData, const uint32_t indicesDataSize) 
{
    CreateVertexBuffer(physicalDevice, device, allocator, commandPool, queue, vertexData, vertexDataSize);
    CreateIndexBuffer(physicalDevice, device, allocator, commandPool, queue, indexData, indicesDataSize);

}

//---------------------------------------------------------------------------------------------------------------------
void Mesh::CleanUp(const VkDevice device, VkAllocationCallbacks* allocator)
{
    //Vertex and Index Buffers
    SAFE_DESTROY_BUFFER(device, m_vb, allocator);
    SAFE_FREE_MEMORY(device, m_vbMemory, allocator);
    SAFE_DESTROY_BUFFER(device, m_ib, allocator);
    SAFE_FREE_MEMORY(device, m_ibMemory, allocator);
}

//---------------------------------------------------------------------------------------------------------------------

void Mesh::CreateVertexBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        VkAllocationCallbacks* allocator,  const VkCommandPool commandPool, VkQueue queue, 
        const char* vertexData, const uint32_t vertexDataSize) 
{


    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: to write from the CPU.
    //VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: ensure that the driver is aware of our copying. Alternative: use flush
    GraphicsUtility::CreateBuffer(physicalDevice, device, allocator, vertexDataSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        &stagingBuffer, &stagingBufferMemory);

    //Filling Vertex Buffer
    GraphicsUtility::CopyCPUDataToBuffer(device,vertexData,stagingBufferMemory,vertexDataSize);

    //VK_BUFFER_USAGE_TRANSFER_DST_BIT: destination in a memory transfer
    GraphicsUtility::CreateBuffer(physicalDevice, device, allocator, vertexDataSize, 
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                 &m_vb, &m_vbMemory);

    //Copy buffer
    GraphicsUtility::CopyBuffer(device, commandPool, queue,
        stagingBuffer, m_vb,vertexDataSize
    );

    vkDestroyBuffer(device, stagingBuffer, allocator);
    vkFreeMemory(device, stagingBufferMemory, allocator);

}

//---------------------------------------------------------------------------------------------------------------------

void Mesh::CreateIndexBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        VkAllocationCallbacks* allocator,  const VkCommandPool commandPool, VkQueue queue, 
        const char* indicesData, const uint32_t indicesDataSize) 
{

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    GraphicsUtility::CreateBuffer(physicalDevice, device, allocator, indicesDataSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        &stagingBuffer, &stagingBufferMemory
    );

    GraphicsUtility::CopyCPUDataToBuffer(device, indicesData,stagingBufferMemory,indicesDataSize);

    GraphicsUtility::CreateBuffer(physicalDevice, device,allocator, indicesDataSize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_ib, &m_ibMemory);

    //Copy buffer
    GraphicsUtility::CopyBuffer(device, commandPool, queue,
        stagingBuffer, m_ib,indicesDataSize
    );

    vkDestroyBuffer(device, stagingBuffer, allocator);
    vkFreeMemory(device, stagingBufferMemory, allocator);
}
