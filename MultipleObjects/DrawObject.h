#pragma once

#include <vulkan/vulkan.h> 

#include <vector>

#include "MVPUniform.h"

class Texture;

class DrawObject {

public: 
    DrawObject();
    
    void Init(const VkDevice device,VkAllocationCallbacks* allocator, const Texture* texture);
    void CleanUp(const VkDevice device,VkAllocationCallbacks* allocator);
    
    //Swap chain
    void RecreateSwapChainObjects(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        VkAllocationCallbacks* allocator, const VkDescriptorPool descriptorPool, 
        const uint32_t numImages, const VkDescriptorSetLayout  descriptorSetLayout);
    void CleanUpSwapChainObjects(const VkDevice device,VkAllocationCallbacks* allocator, const uint32_t numImages);

    inline void SetPos(const glm::vec3& pos);
    inline void SetPos(const float x, const float y, const float z);
    void SetProj(const float perspective);

    void UpdateUniformBuffers(const VkDevice device, const uint32_t imageIndex);

    //inline uint32_t GetIndicesSize() const;
    //inline VkDescriptorSetLayout GetDescriptorSetLayout() const;         
    inline const VkDescriptorSet GetDescriptorSet(const uint32_t idx) const;

private:

    void CreateUniformBuffers(const VkPhysicalDevice physicalDevice, VkDevice device,VkAllocationCallbacks* allocator, 
        const uint32_t numImages);
    void CreateDescriptorSets(const VkDevice device, const VkDescriptorPool descriptorPool, 
        const uint32_t numImages, const VkDescriptorSetLayout  descriptorSetLayout);

    glm::vec3                      m_pos;
    MVPUniform                     m_mvpMat;
    std::vector<VkDescriptorSet>   m_descriptorSets; //To bind uniform buffers

    const Texture*                 m_texture;

    //These Uniform buffers will be updated in every DrawFrame
    std::vector<VkBuffer>          m_uniformBuffers;
    std::vector<VkDeviceMemory>    m_uniformBuffersMemory;

    //VkDescriptorSetLayout          m_descriptorSetLayout;
    //VkBuffer            m_vb;
    //VkBuffer            m_ib;
};

//---------------------------------------------------------------------------------------------------------------------

void DrawObject::SetPos(const float x, const float y, const float z) { m_pos = glm::vec3(x,y,z); }
void DrawObject::SetPos(const glm::vec3& pos) { m_pos = pos; }
const VkDescriptorSet DrawObject::GetDescriptorSet(const uint32_t idx) const { return m_descriptorSets[idx]; }

//VkDescriptorSetLayout DrawObject::GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
