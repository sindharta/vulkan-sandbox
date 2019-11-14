
#include "DrawObject.h"
#include <array>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp> //glm::rotate, glm::lookAt, glm::perspective

#include "Utilities/Macros.h"
#include "Utilities/GraphicsUtility.h"

#include "Texture.h"
#include "Mesh.h"
namespace Shin {

DrawObject::DrawObject() {

}

//---------------------------------------------------------------------------------------------------------------------

void DrawObject::Init(const VkDevice device,VkAllocationCallbacks* allocator, const Mesh* mesh, const Texture* texture) 
{
    m_mesh = mesh;
    m_texture = texture;
    m_mvpMat.ViewMat  = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}

//---------------------------------------------------------------------------------------------------------------------

void DrawObject::CleanUp(const VkDevice device, VkAllocationCallbacks* allocator) {
    m_mesh = nullptr;
    m_texture = nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
void DrawObject::RecreateSwapChainObjects(const VkPhysicalDevice physicalDevice, const VkDevice device, 
    VkAllocationCallbacks* allocator, const VkDescriptorPool descriptorPool,
    const uint32_t numImages, const VkDescriptorSetLayout  descriptorSetLayout) 
{
    CreateUniformBuffers(physicalDevice, device,allocator, numImages);
    CreateDescriptorSets(device, descriptorPool, numImages, descriptorSetLayout);
}

//---------------------------------------------------------------------------------------------------------------------
void DrawObject::CleanUpSwapChainObjects(const VkDevice device, VkAllocationCallbacks* allocator) 
{
    const uint32_t numImages =static_cast<uint32_t>(m_uniformBuffers.size());
    for (size_t i = 0; i < numImages; i++) {
        vkDestroyBuffer(device, m_uniformBuffers[i], allocator);
        vkFreeMemory(device, m_uniformBuffersMemory[i], allocator);
    }
    m_uniformBuffers.clear();
    m_uniformBuffersMemory.clear();

}

//---------------------------------------------------------------------------------------------------------------------
void DrawObject::SetProj(const float perspective) {
    m_mvpMat.ProjMat = glm::perspective(glm::radians(45.0f), perspective, 0.1f, 10.0f);
    m_mvpMat.ProjMat[1][1] *= -1; //flip Y axis
}

//---------------------------------------------------------------------------------------------------------------------

void DrawObject::UpdateUniformBuffers(const VkDevice device, const uint32_t imageIndex) {

    static const auto START_TIME = std::chrono::high_resolution_clock::now();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - START_TIME).count();

    glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), m_pos);
    m_mvpMat.ModelMat = translationMat * glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    GraphicsUtility::CopyCPUDataToBuffer(device, &m_mvpMat, m_uniformBuffersMemory[imageIndex],sizeof(m_mvpMat));

}


//---------------------------------------------------------------------------------------------------------------------
void DrawObject::CreateUniformBuffers(const VkPhysicalDevice physicalDevice, VkDevice device,VkAllocationCallbacks* allocator, 
        const uint32_t numImages) {

    const VkDeviceSize bufferSize = sizeof(MVPUniform);

    m_uniformBuffers.resize(numImages);
    m_uniformBuffersMemory.resize(numImages);

    //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: to write from the CPU.
    //VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: ensure that the driver is aware of our copying. Alternative: use flush
    for (uint32_t i = 0; i < numImages; ++i) {
        GraphicsUtility::CreateBuffer(physicalDevice, device, allocator, bufferSize, 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            &m_uniformBuffers[i], &m_uniformBuffersMemory[i]
        );
    }
    
}

//---------------------------------------------------------------------------------------------------------------------
void DrawObject::CreateDescriptorSets(const VkDevice device, const VkDescriptorPool descriptorPool, 
    const uint32_t numImages, const VkDescriptorSetLayout  descriptorSetLayout) 
{

    std::vector<VkDescriptorSetLayout> layouts(numImages, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = numImages;
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(numImages);
    if (vkAllocateDescriptorSets(device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    if (nullptr != m_texture) {
        for (size_t i = 0; i < numImages; ++i) {

            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = m_uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(MVPUniform);

            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_texture->GetImageView();
            imageInfo.sampler   = m_texture->GetSampler();

            std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), 
                descriptorWrites.data(), 0, nullptr
            );
        }

    } else {
        for (size_t i = 0; i < numImages; ++i) {

            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = m_uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(MVPUniform);

            std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), 
                descriptorWrites.data(), 0, nullptr
            );
        }

    }


}


} //end namespace
