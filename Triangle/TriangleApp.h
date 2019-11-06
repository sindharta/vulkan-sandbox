#pragma once

#include <vulkan/vulkan.h> 
#include <glm/vec4.hpp>
#include <vector>
#include <map>
#include "Config.h"
#include "VulkanDebugMessenger.h"
#include "QueueFamilyIndices.h"

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

    void Loop(); 
    void PrintSupportedExtensions();
    void GetRequiredExtensionsInto(std::vector<const char*>* extensions);
    static void GetVulkanQueueFamilyPropertiesInto(const VkPhysicalDevice& device, std::vector<VkQueueFamilyProperties>* );
    static QueueFamilyIndices ExtractVulkanQueueFamilyIndices(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

    GLFWwindow* m_window;

    VkInstance          m_vulkanInstance;
    VkSurfaceKHR        m_vulkanSurface;
    VkPhysicalDevice    m_vulkanPhysicalDevice;
    VkDevice            m_vulkanLogicalDevice;
    QueueFamilyIndices  m_vulkanQueueFamilyIndices;

    //Queues
    VkQueue             m_vulkanGraphicsQueue;
    VkQueue             m_vulkanPresentationQueue;

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
};