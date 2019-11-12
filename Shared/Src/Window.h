#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h> 

struct GLFWwindow;

class Window {
    public:
        Window();
        void Init(const uint32_t width, const uint32_t height, void(*resizedCallback)(void*), void* resizedCallbackData);
        void CleanUp();

        VkResult CreateVulkanSurfaceInto(const VkInstance vulkanInstance, VkAllocationCallbacks* allocator, 
            VkSurfaceKHR* vulkanSurface) const;
        VkExtent2D  SelectVulkanSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

        bool Loop() const; 
        void WaitInMinimizedState() const;
        void OnWindowResized();

    private:
        GLFWwindow* m_glfwWindow;
        void(* m_windowResizedCallback)(void*);
        void* m_windowResizedCallbackData;


};