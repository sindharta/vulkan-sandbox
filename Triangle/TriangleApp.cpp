#include "TriangleApp.h"
#include <GLFW/glfw3.h>
#include <stdexcept> //std::runtime_error
#include <iostream> //cout

//---------------------------------------------------------------------------------------------------------------------

#ifdef ENABLE_VULKAN_VALIDATION_LAYERS

const std::vector<const char*> g_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#endif //NDEBUG


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


    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef ENABLE_VULKAN_VALIDATION_LAYERS
    if (!CheckValidationLayers()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
    createInfo.ppEnabledLayerNames = g_validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif //ENABLE_VULKAN_VALIDATION_LAYERS

#ifdef ENABLE_VULKAN_DEBUG //Check Debug Utils before creating instance
    const VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo 
        = static_cast<const VulkanDebugMessenger>(m_vulkanDebug).GetCreateInfo();
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
#endif //ENABLE_VULKAN_DEBUG

    if (vkCreateInstance(&createInfo, nullptr, &m_vulkanInstance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

}


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

bool TriangleApp::CheckValidationLayers() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


    for (const char* layerName : g_validationLayers) {
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

void TriangleApp::CleanUp() {
#ifdef ENABLE_VULKAN_DEBUG
    m_vulkanDebug.Shutdown(m_vulkanInstance);
#endif
    
    vkDestroyInstance(m_vulkanInstance, nullptr);
    glfwDestroyWindow(m_window);
    glfwTerminate();
}
