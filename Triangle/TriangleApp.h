#pragma once

#include <vulkan/vulkan.h> 
#include <glm/vec4.hpp>

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

    GLFWwindow* m_window;
    VkInstance m_vulkanInstance;

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
};