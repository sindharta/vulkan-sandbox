#pragma once

#include <vulkan/vulkan.h> 

#include <vector>

#include "MVPUniform.h"

namespace Shin {

class Texture;
class Mesh;
class OffScreenPass;

class DrawObject {

public: 
    DrawObject();
    
    //[TODO-sin: 2019-11-14] Probably better to generalize so that we can add Texture, pass into a vector,
    //and generate descriptor set dynamically
    void Init(const VkDevice device,VkAllocationCallbacks* allocator, const Mesh* mesh, const Texture* texture);
    void Init(const VkDevice device,VkAllocationCallbacks* allocator, const Mesh* mesh, const OffScreenPass* pass);
    void CleanUp(const VkDevice device,VkAllocationCallbacks* allocator);
    
    //Swap chain
    void RecreateSwapChainObjects(const VkPhysicalDevice physicalDevice, const VkDevice device, 
        VkAllocationCallbacks* allocator, const VkDescriptorPool descriptorPool, 
        const uint32_t numImages, const VkDescriptorSetLayout  descriptorSetLayout);
    void CleanUpSwapChainObjects(const VkDevice device,VkAllocationCallbacks* allocator);

    inline void SetPos(const glm::vec3& pos);
    inline void SetPos(const float x, const float y, const float z);
    void SetProj(const float perspective);

    void UpdateUniformBuffers(const VkDevice device, const uint32_t imageIndex);

    inline const VkDescriptorSet GetDescriptorSet(const uint32_t idx) const;
    inline const Mesh* GetMesh() const;

private:

    void CreateUniformBuffers(const VkPhysicalDevice physicalDevice, VkDevice device,VkAllocationCallbacks* allocator, 
        const uint32_t numImages);
    void CreateDescriptorSets(const VkDevice device, const VkDescriptorPool descriptorPool, 
        const uint32_t numImages, const VkDescriptorSetLayout  descriptorSetLayout);

    glm::vec3                      m_pos;
    MVPUniform                     m_mvpMat;
    std::vector<VkDescriptorSet>   m_descriptorSets; //To bind uniform buffers. One per image in swap chain

    const Texture*                 m_texture;
    const OffScreenPass*           m_offScreenPass;
    const Mesh*                    m_mesh;

    //These Uniform buffers will be updated in every DrawFrame
    std::vector<VkBuffer>          m_uniformBuffers;
    std::vector<VkDeviceMemory>    m_uniformBuffersMemory;

};

//---------------------------------------------------------------------------------------------------------------------

void DrawObject::SetPos(const float x, const float y, const float z) { m_pos = glm::vec3(x,y,z); }
void DrawObject::SetPos(const glm::vec3& pos) { m_pos = pos; }
const VkDescriptorSet DrawObject::GetDescriptorSet(const uint32_t idx) const { return m_descriptorSets[idx]; }
const Mesh* DrawObject::GetMesh() const { return m_mesh; }

};
