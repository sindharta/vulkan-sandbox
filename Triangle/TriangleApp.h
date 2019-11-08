#pragma once

#include <vulkan/vulkan.h> 
#include <glm/vec4.hpp>
#include <vector>
#include <map>

#include "Config.h"
#include "VulkanDebugMessenger.h"
#include "QueueFamilyIndices.h"
#include "PhysicalDeviceSurfaceInfo.h"

struct GLFWwindow;

class TriangleApp {
public:
    TriangleApp();
    void Run();
    void CleanUp();
    inline void RequestToRecreateSwapChain();

private:
    void InitWindow();
    void InitVulkan();
    void RecreateVulkanSwapChain();

#ifdef ENABLE_VULKAN_DEBUG
    void InitVulkanDebugInstance(const VkApplicationInfo& appInfo, const std::vector<const char*>& extensions);
    bool CheckRequiredVulkanLayersAvailability(const std::vector<const char*> requiredLayers);
    VulkanDebugMessenger m_vulkanDebug;
#else
    void InitVulkanInstance(const VkApplicationInfo& appInfo, const std::vector<const char*>& extensions);

#endif //ENABLE_VULKAN_DEBUG
    
    void CreateVulkanSurface();
    void PickVulkanPhysicalDevice();
    void CreateVulkanLogicalDevice();
    void CreateVulkanSwapChain();
    void CreateVulkanImageViews();
    void CreateVulkanRenderPass();
    void CreateVulkanDescriptorSetLayout();
    void CreateVulkanDescriptorPool();
    void CreateVulkanDescriptorSets();
    void CreateVulkanGraphicsPipeline();
    void CreateVulkanFrameBuffers();
    void CreateVulkanCommandPool();
    void CreateVulkanVertexBuffer();
    void CreateVulkanIndexBuffer();
    void CreateVulkanTextureImage();
    void CreateVulkanTextureImageView();
    void CreateVulkanUniformBuffers();
    void CreateVulkanCommandBuffers();
    void CreateVulkanSyncObjects();


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
    static VkSurfaceFormatKHR PickVulkanSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* availableFormats);
    static VkPresentModeKHR   PickVulkanSwapPresentMode(const std::vector<VkPresentModeKHR>* availableModes);
    static VkExtent2D         PickVulkanSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);

    void CleanUpVulkanSwapChain();

    GLFWwindow* m_window;

    VkInstance                      m_vulkanInstance;
    VkSurfaceKHR                    m_vulkanSurface;
    VkPhysicalDevice                m_vulkanPhysicalDevice;
    VkDevice                        m_vulkanLogicalDevice;
    VkSwapchainKHR                  m_vulkanSwapChain;
    VkRenderPass                    m_vulkanRenderPass;
    VkDescriptorSetLayout           m_vulkanDescriptorSetLayout;
    VkDescriptorPool                m_vulkanDescriptorPool; //A pool to create descriptor set to bind uniform buffers 
    std::vector<VkDescriptorSet>    m_vulkanDescriptorSets; //To bind uniform buffers
    VkPipelineLayout                m_vulkanPipelineLayout; //to pass uniform values to shaders
    VkPipeline                      m_vulkanGraphicsPipeline;
    VkCommandPool                   m_vulkanCommandPool;

    VkBuffer            m_vulkanVB;
    VkDeviceMemory      m_vulkanVBMemory;
    VkBuffer            m_vulkanIB;
    VkDeviceMemory      m_vulkanIBMemory;
    VkImage             m_vulkanTextureImage;
    VkDeviceMemory      m_vulkanTextureImageMemory;
    VkImageView         m_vulkanTextureImageView;

    std::vector<VkCommandBuffer> m_vulkanCommandBuffers;
    
    std::vector<VkSemaphore> m_vulkanImageAvailableSemaphores;
    std::vector<VkSemaphore> m_vulkanRenderFinishedSemaphores;

    //Swap chain
    std::vector<VkImage>        m_vulkanSwapChainImages;
    std::vector<VkImageView>    m_vulkanSwapChainImageViews;
    VkFormat                    m_vulkanSwapChainSurfaceFormat;
    VkExtent2D                  m_vulkanSwapChainExtent;
    std::vector<VkFramebuffer>  m_vulkanSwapChainFramebuffers;
    uint32_t                    m_vulkanCurrentFrame;
    std::vector<VkFence>        m_vulkanInFlightFences; //CPU-GPU synchronizations
    std::vector<VkFence>        m_vulkanImagesInFlight; //To test if the current frame is still in flight
    bool                        m_recreateSwapChainRequested;

    //These Uniform buffers will be updated in every DrawFrame
    std::vector<VkBuffer>       m_vulkanUniformBuffers;
    std::vector<VkDeviceMemory> m_vulkanUniformBuffersMemory;

    //Queues
    QueueFamilyIndices  m_vulkanQueueFamilyIndices;
    VkQueue             m_vulkanGraphicsQueue;
    VkQueue             m_vulkanPresentationQueue;

    static const uint32_t WIDTH = 800;
    static const uint32_t HEIGHT = 600;

    const uint32_t MAX_FRAMES_IN_FLIGHT = 100;
};

void TriangleApp::RequestToRecreateSwapChain() { m_recreateSwapChainRequested = true; }
