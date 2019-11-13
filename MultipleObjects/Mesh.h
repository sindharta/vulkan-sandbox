#pragma once

#include <vulkan/vulkan.h> 

class Mesh {

public:
    Mesh();
    void Init(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        VkAllocationCallbacks* allocator,  const VkCommandPool commandPool, VkQueue queue, 
        const char* vertexData, const uint32_t vertexDataSize, const char* indexData, const uint32_t indexDataSize,
        const uint32_t numIndices);

    void CleanUp(const VkDevice device, VkAllocationCallbacks* allocator);

    inline VkBuffer GetVertexBuffer() const;
    inline VkBuffer GetIndexBuffer() const;

    inline uint32_t GetNumIndices() const;
private:
    void CreateVertexBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        VkAllocationCallbacks* allocator,  const VkCommandPool commandPool, VkQueue queue, 
        const char* vertexData, const uint32_t vertexDataSize);

    void CreateIndexBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        VkAllocationCallbacks* allocator,  const VkCommandPool commandPool, VkQueue queue, 
        const char* indicesData, const uint32_t indicesDataSize);

    VkBuffer            m_vb;
    VkDeviceMemory      m_vbMemory;
    VkBuffer            m_ib;
    VkDeviceMemory      m_ibMemory;
    uint32_t            m_numIndices;
};

//---------------------------------------------------------------------------------------------------------------------

VkBuffer Mesh::GetVertexBuffer() const { return m_vb; }
VkBuffer Mesh::GetIndexBuffer() const { return m_ib; }
uint32_t Mesh::GetNumIndices() const { return m_numIndices; }
