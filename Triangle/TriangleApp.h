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
    void Loop(); 
    void CleanUp();
    void PrintSupportedExtensions();
    bool CheckValidationLayers();
    void GetRequiredExtensionsInto(std::vector<const char*>& extensions);

    GLFWwindow* m_window;
    VkInstance m_vulkanInstance;
#ifdef ENABLE_VULKAN_DEBUG
    VulkanDebugMessenger m_vulkanDebug;
#endif

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
};