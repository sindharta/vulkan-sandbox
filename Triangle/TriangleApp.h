#pragma once

#include <vulkan/vulkan.h> 
#include <glm/vec4.hpp>
#include <vector>
#include "Config.h"
#include "VulkanDebugMessenger.h"

struct GLFWwindow;

class TriangleApp {
public:
    void Run();

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

    void Loop(); 
    void CleanUp();
    void PrintSupportedExtensions();
    void GetRequiredExtensionsInto(std::vector<const char*>& extensions);

    GLFWwindow* m_window;
    VkInstance m_vulkanInstance;


    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
};