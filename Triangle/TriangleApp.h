#pragma once

#include <vulkan/vulkan.h> 
#include <glm/vec4.hpp>
#include <vector>
#include <map>
#include "Config.h"
#include "VulkanDebugMessenger.h"

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
    
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void Loop(); 
    void PrintSupportedExtensions();
    void GetRequiredExtensionsInto(std::vector<const char*>* extensions);
    void GetVulkanQueueFamilyPropertiesInto(const VkPhysicalDevice& device, std::vector<VkQueueFamilyProperties>* );
    static bool IsVulkanDeviceValid(const std::vector<VkQueueFamilyProperties>*, const VkQueueFlags queueFlags);
    void UpdateQueueFamilyPropertiesMapping(const std::vector<VkQueueFlagBits>* requiredQueueFlags);

    GLFWwindow* m_window;
    VkInstance m_vulkanInstance;
    VkPhysicalDevice m_vulkanPhysicalDevice;
    VkDevice m_vulkanLogicalDevice;
    std::vector<VkQueueFamilyProperties> m_vulkanQueueFamilyProperties;
    std::map<VkQueueFlagBits, uint32_t> m_vulkanQueueFamilyIndexMap;



    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
};