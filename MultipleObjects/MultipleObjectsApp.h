#pragma once

#include <vulkan/vulkan.h> 
#include <glm/vec4.hpp>
#include <vector>
#include <map>

//Shared
#include "SharedConfig.h"
#include "VulkanDebugMessenger.h"
#include "PhysicalDeviceSurfaceInfo.h"

#include "QueueFamilyIndices.h"
#include "DrawObject.h"

class Window;
class Texture;
class Mesh;
class DrawPipeline;

class MultipleObjectsApp {
public:
    MultipleObjectsApp();
    void Run();
    void CleanUp();
    inline void RequestToRecreateSwapChain();

private:
    void InitVulkan();
    void RecreateSwapChain();

#ifdef ENABLE_VULKAN_DEBUG
    void InitVulkanDebugInstance(const VkApplicationInfo& appInfo, const std::vector<const char*>& extensions);
    bool CheckRequiredVulkanLayersAvailability(const std::vector<const char*> requiredLayers);
    VulkanDebugMessenger m_Debug;
#else
    void InitVulkanInstance(const VkApplicationInfo& appInfo, const std::vector<const char*>& extensions);

#endif //ENABLE_VULKAN_DEBUG
    
    void PickPhysicalDevice();
    void CreateLogicalDevice();

    void CreateDescriptorSetLayout();
    void CreateCommandPool();
    //void CreateVertexBuffer();
    //void CreateIndexBuffer();
    void CreateSyncObjects();
    
    //Swap chain related
    void CreateSwapChain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateFrameBuffers();
    void CreateDescriptorPool();
    void CreateCommandBuffers();


    void Loop(); 
    void DrawFrame();
    void UpdateVulkanUniformBuffers(uint32_t imageIndex);

    static void PrintSupportedExtensions();
    static void GetRequiredExtensionsInto(std::vector<const char*>* extensions);

    static void GetVulkanQueueFamilyPropertiesInto(const VkPhysicalDevice& device, std::vector<VkQueueFamilyProperties>* );
    static QueueFamilyIndices QueryVulkanQueueFamilyIndices(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
    static PhysicalDeviceSurfaceInfo QueryVulkanPhysicalDeviceSurfaceInfo(const VkPhysicalDevice& device, 
        const VkSurfaceKHR& surface);
    static bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device,const std::vector<const char*>* extensions);


    //Swap chain
    static VkSurfaceFormatKHR PickSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* availableFormats);
    static VkPresentModeKHR   PickSwapPresentMode(const std::vector<VkPresentModeKHR>* availableModes);

    void CleanUpVulkanSwapChain();


    Window*                         m_window;

    VkInstance                      m_instance;
    VkSurfaceKHR                    m_surface;
    VkPhysicalDevice                m_physicalDevice;
    VkDevice                        m_logicalDevice;
    VkSwapchainKHR                  m_swapChain;
    VkRenderPass                    m_renderPass;
    VkDescriptorSetLayout           m_texDescriptorSetLayout;
    VkDescriptorSetLayout           m_colorDescriptorSetLayout;
    VkDescriptorPool                m_descriptorPool; //A pool to create descriptor set to bind uniform buffers 

    std::vector<DrawObject>         m_drawObjects; // multiple objects

    std::vector<DrawPipeline*>      m_drawPipelines;
    VkCommandPool                   m_commandPool;

    Mesh*                       m_texMesh;
    Mesh*                       m_colorMesh;
    Texture*                    m_texture;

    std::vector<VkCommandBuffer> m_commandBuffers;
    
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;

    //Swap chain
    std::vector<VkImage>        m_swapChainImages;
    std::vector<VkImageView>    m_swapChainImageViews;
    VkFormat                    m_swapChainSurfaceFormat;
    VkExtent2D                  m_swapChainExtent;
    std::vector<VkFramebuffer>  m_swapChainFramebuffers;
    uint32_t                    m_currentFrame;
    std::vector<VkFence>        m_inFlightFences; //CPU-GPU synchronizations
    std::vector<VkFence>        m_imagesInFlight; //To test if the current frame is still in flight
    bool                        m_recreateSwapChainRequested;

    //Queues
    QueueFamilyIndices  m_queueFamilyIndices;
    VkQueue             m_graphicsQueue;
    VkQueue             m_presentationQueue;

    static const uint32_t WIDTH = 800;
    static const uint32_t HEIGHT = 600;

    const uint32_t MAX_FRAMES_IN_FLIGHT = 100;
};

void MultipleObjectsApp::RequestToRecreateSwapChain() { m_recreateSwapChainRequested = true; }
