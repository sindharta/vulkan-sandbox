#include "TriangleApp.h"
#include <GLFW/glfw3.h>
#include <stdexcept> //std::runtime_error
#include <iostream> //cout


//---------------------------------------------------------------------------------------------------------------------

TriangleApp::TriangleApp() : m_vulkanPhysicalDevice(VK_NULL_HANDLE), m_window(nullptr) {
}

//---------------------------------------------------------------------------------------------------------------------


void TriangleApp::Run() {
    InitWindow();
    PrintSupportedExtensions();
    InitVulkan();

#ifdef ENABLE_VULKAN_DEBUG
    if (VK_SUCCESS != m_vulkanDebug.Init(m_vulkanInstance)) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
    
#endif //ENABLE_VULKAN_DEBUG

    Loop();
    CleanUp();
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::InitWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan window", nullptr, nullptr);

}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::InitVulkan() {

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    //Get required extension to create Vulkan with GLFW
    std::vector<const char*> extensions;
    GetRequiredExtensionsInto(extensions);

#ifdef ENABLE_VULKAN_DEBUG
    InitVulkanDebugInstance(appInfo, extensions);
#else
    InitVulkanInstance(appInfo, extensions);
#endif

    //Pick physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, devices.data());

    for (const VkPhysicalDevice& device : devices) {
        if (IsVulkanDeviceValid(device, VK_QUEUE_GRAPHICS_BIT)) {
            m_vulkanPhysicalDevice = device;
            break;
        }
    }

    if (m_vulkanPhysicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

}

#ifdef ENABLE_VULKAN_DEBUG

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::InitVulkanDebugInstance(const VkApplicationInfo& appInfo, const std::vector<const char*>& extensions) 
{
    const std::vector<const char*> requiredLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    const VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo 
        = static_cast<const VulkanDebugMessenger>(m_vulkanDebug).GetCreateInfo();

    if (!CheckRequiredVulkanLayersAvailability(requiredLayers)) {
        throw std::runtime_error("Required Vulkan layers are requested, but some are not available!");
    }
    createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
    createInfo.ppEnabledLayerNames = requiredLayers.data();

    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;

    if (vkCreateInstance(&createInfo, nullptr, &m_vulkanInstance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool TriangleApp::CheckRequiredVulkanLayersAvailability(const std::vector<const char*> requiredLayers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : requiredLayers) {
        bool layerFound = false;

        for (const VkLayerProperties& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

#else

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::InitVulkanInstance(const VkApplicationInfo& appInfo, const std::vector<const char*>& extensions) {


    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &m_vulkanInstance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

#endif //ENABLE_VULKAN_DEBUG



//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::Loop() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::PrintSupportedExtensions() {
    
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::cout << extensionCount << " extensions supported" << std::endl;
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    for (const auto& extension : extensions) {
        std::cout << "\t" << extension.extensionName << std::endl;
    }
}
//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::GetRequiredExtensionsInto(std::vector<const char*>& extensions) {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    extensions.insert(extensions.end(), &glfwExtensions[0], &glfwExtensions[glfwExtensionCount]);

#ifdef ENABLE_VULKAN_DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

}

//---------------------------------------------------------------------------------------------------------------------

bool TriangleApp::IsVulkanDeviceValid(const VkPhysicalDevice& device, const VkQueueFlags queueFlags) {

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
        if ((queueFamily.queueFlags & queueFlags) == queueFlags) {
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CleanUp() {
#ifdef ENABLE_VULKAN_DEBUG
    m_vulkanDebug.Shutdown(m_vulkanInstance);
#endif

    if (nullptr != m_vulkanInstance) {
        vkDestroyInstance(m_vulkanInstance, nullptr);
        m_vulkanInstance = nullptr;
    }

    if (nullptr != m_window) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = nullptr;
    }
   
}
