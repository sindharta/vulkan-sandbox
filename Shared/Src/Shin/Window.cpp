#include "Window.h"
#include <GLFW/glfw3.h>
#include <algorithm>  //max

static void OnWindowResized(GLFWwindow* glfwWindow, int width, int height) {
    Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    window->OnWindowResized();
}

//---------------------------------------------------------------------------------------------------------------------

Window::Window() :m_glfwWindow(nullptr), m_windowResizedCallback(nullptr), m_windowResizedCallbackData(nullptr) {

}

//---------------------------------------------------------------------------------------------------------------------

void Window::Init(const uint32_t width, const uint32_t height, void(*resizedCallback)(void*), void* resizedCallbackData)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_glfwWindow = glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr);
    glfwSetWindowUserPointer(m_glfwWindow, this);
    glfwSetFramebufferSizeCallback(m_glfwWindow, ::OnWindowResized);
    m_windowResizedCallback = resizedCallback;
    m_windowResizedCallbackData = resizedCallbackData;
}

//---------------------------------------------------------------------------------------------------------------------

void Window::CleanUp() 
{
    if (nullptr != m_glfwWindow) {
        glfwDestroyWindow(m_glfwWindow);
        glfwTerminate();
        m_glfwWindow = nullptr;
    }

}

//---------------------------------------------------------------------------------------------------------------------

VkResult Window::CreateVulkanSurfaceInto(const VkInstance vulkanInstance, VkAllocationCallbacks* allocator, 
            VkSurfaceKHR* vulkanSurface) const
{ 
    return glfwCreateWindowSurface(vulkanInstance, m_glfwWindow, allocator, vulkanSurface);
}

//---------------------------------------------------------------------------------------------------------------------

VkExtent2D  Window::SelectVulkanSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const{
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(m_glfwWindow, &width, &height);

        VkExtent2D actualExtent = {
            std::max(capabilities.minImageExtent.width, 
                     std::min(capabilities.maxImageExtent.width, static_cast<uint32_t>(width))
                    ),
            std::max(capabilities.minImageExtent.height, 
                     std::min(capabilities.maxImageExtent.height, static_cast<uint32_t>(height))
                    )
        };

        return actualExtent;
    }

}

//---------------------------------------------------------------------------------------------------------------------

bool Window::Loop() const {
    const bool loop = !glfwWindowShouldClose(m_glfwWindow);
//    if (loop) 
    {
        glfwPollEvents();
    }
    return loop;
}

//---------------------------------------------------------------------------------------------------------------------

void Window::WaitInMinimizedState() const{
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_glfwWindow, &width, &height);
        glfwWaitEvents();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void Window::OnWindowResized() {
    if (nullptr != m_windowResizedCallback) {
        m_windowResizedCallback(m_windowResizedCallbackData);
    }
}
