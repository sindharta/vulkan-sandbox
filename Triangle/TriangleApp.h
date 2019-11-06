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

private:
    void InitWindow();
    void InitVulkan();

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
    void CreateVulkanGraphicsPipeline();
    void CreateVulkanFrameBuffers();
    void CreateVulkanCommandPool();
    void CreateVulkanCommandBuffers();
    void CreateVulkanSemaphores();

    void Loop(); 
    void DrawFrame();
    void PrintSupportedExtensions();
    void GetRequiredExtensionsInto(std::vector<const char*>* extensions);
    static void GetVulkanQueueFamilyPropertiesInto(const VkPhysicalDevice& device, std::vector<VkQueueFamilyProperties>* );
    static QueueFamilyIndices QueryVulkanQueueFamilyIndices(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
    static PhysicalDeviceSurfaceInfo QueryVulkanPhysicalDeviceSurfaceInfo(const VkPhysicalDevice& device, 
        const VkSurfaceKHR& surface);
    static bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device,const std::vector<const char*>* extensions);

    //Swap chain
    static VkSurfaceFormatKHR PickVulkanSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* availableFormats);
    static VkPresentModeKHR   PickVulkanSwapPresentMode(const std::vector<VkPresentModeKHR>* availableModes);
    static VkExtent2D         PickVulkanSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    GLFWwindow* m_window;

    VkInstance          m_vulkanInstance;
    VkSurfaceKHR        m_vulkanSurface;
    VkPhysicalDevice    m_vulkanPhysicalDevice;
    VkDevice            m_vulkanLogicalDevice;
    VkSwapchainKHR      m_vulkanSwapChain;
    VkRenderPass        m_vulkanRenderPass;
    VkPipelineLayout    m_vulkanPipelineLayout;
    VkPipeline          m_vulkanGraphicsPipeline;
    VkCommandPool       m_vulkanCommandPool;
    std::vector<VkCommandBuffer> m_vulkanCommandBuffers;
    VkSemaphore m_vulkanImageAvailableSemaphore;
    VkSemaphore m_vulkanRenderFinishedSemaphore;

    //Swap chain
    std::vector<VkImage>        m_vulkanSwapChainImages;
    std::vector<VkImageView>    m_vulkanSwapChainImageViews;
    VkFormat                    m_vulkanSwapChainSurfaceFormat;
    VkExtent2D                  m_vulkanSwapChainExtent;
    std::vector<VkFramebuffer>  m_vulkanSwapChainFramebuffers;

    //Queues
    QueueFamilyIndices  m_vulkanQueueFamilyIndices;
    VkQueue             m_vulkanGraphicsQueue;
    VkQueue             m_vulkanPresentationQueue;


    static const uint32_t WIDTH = 800;
    static const uint32_t HEIGHT = 600;
};