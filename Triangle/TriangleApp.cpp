#include "TriangleApp.h"
#include <GLFW/glfw3.h>
#include <stdexcept> //std::runtime_error
#include <iostream> //cout


VkAllocationCallbacks* g_allocator = nullptr; //Always use default allocator

const std::vector<const char*> g_requiredVulkanLayers = {
    "VK_LAYER_KHRONOS_validation"
};

//---------------------------------------------------------------------------------------------------------------------

TriangleApp::TriangleApp() 
    : m_vulkanInstance(nullptr), m_vulkanSurface(nullptr)
    , m_vulkanPhysicalDevice(VK_NULL_HANDLE), m_vulkanLogicalDevice(nullptr), m_vulkanGraphicsQueue(nullptr)
    , m_window(nullptr) 
{
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
    GetRequiredExtensionsInto(&extensions);

    std::cout << "Extensions used for initializing: " << std::endl;
    for (const char* extension : extensions) {
        std::cout << "\t" << extension << std::endl;
    }


#ifdef ENABLE_VULKAN_DEBUG
    InitVulkanDebugInstance(appInfo, extensions);
#else
    InitVulkanInstance(appInfo, extensions);
#endif

    PickVulkanPhysicalDevice();

    const std::vector<VkQueueFlagBits> requiredQueueFlags = {VK_QUEUE_GRAPHICS_BIT, };    
    UpdateQueueFamilyPropertiesMapping(&requiredQueueFlags);
    CreateVulkanLogicalDevice();
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::CreateVulkanSurface() {
    if (glfwCreateWindowSurface(m_vulkanInstance, m_window, g_allocator, &m_vulkanSurface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::PickVulkanPhysicalDevice()  {
    //Pick physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, devices.data());

    for (const VkPhysicalDevice& device : devices) {
        GetVulkanQueueFamilyPropertiesInto(device, &m_vulkanQueueFamilyProperties);
        if (IsVulkanDeviceValid(&m_vulkanQueueFamilyProperties, VK_QUEUE_GRAPHICS_BIT)) {
            m_vulkanPhysicalDevice = device;
            break;
        }
    }

    if (m_vulkanPhysicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateVulkanLogicalDevice()  {
    //Create device queue
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_vulkanQueueFamilyIndexMap[VK_QUEUE_GRAPHICS_BIT];
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    //Device features
    VkPhysicalDeviceFeatures deviceFeatures = {};

    //Creating logical device
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0;
    createInfo.enabledLayerCount = 0;

#ifdef ENABLE_VULKAN_DEBUG
    //[Note-sin: 2019-11-5] These settings are ignored by newer implementations of Vulkan
    createInfo.enabledLayerCount = static_cast<uint32_t>(g_requiredVulkanLayers.size());
    createInfo.ppEnabledLayerNames = g_requiredVulkanLayers.data();
#endif

    if (VK_SUCCESS !=vkCreateDevice(m_vulkanPhysicalDevice, &createInfo, nullptr, &m_vulkanLogicalDevice) ) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(m_vulkanLogicalDevice, m_vulkanQueueFamilyIndexMap[VK_QUEUE_GRAPHICS_BIT], 0, &m_vulkanGraphicsQueue);

}

#ifdef ENABLE_VULKAN_DEBUG

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::InitVulkanDebugInstance(const VkApplicationInfo& appInfo, const std::vector<const char*>& extensions) 
{
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    const VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo 
        = static_cast<const VulkanDebugMessenger>(m_vulkanDebug).GetCreateInfo();

    if (!CheckRequiredVulkanLayersAvailability(g_requiredVulkanLayers)) {
        throw std::runtime_error("Required Vulkan layers are requested, but some are not available!");
    }
    createInfo.enabledLayerCount = static_cast<uint32_t>(g_requiredVulkanLayers.size());
    createInfo.ppEnabledLayerNames = g_requiredVulkanLayers.data();

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

void TriangleApp::GetRequiredExtensionsInto(std::vector<const char*>* extensions) {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    extensions->insert(extensions->end(), &glfwExtensions[0], &glfwExtensions[glfwExtensionCount]);

#ifdef ENABLE_VULKAN_DEBUG
    extensions->push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::GetVulkanQueueFamilyPropertiesInto(const VkPhysicalDevice& device, 
                                                     std::vector<VkQueueFamilyProperties>* queueFamilies) 
{
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    queueFamilies->resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies->data());
}

//---------------------------------------------------------------------------------------------------------------------

bool TriangleApp::IsVulkanDeviceValid(const std::vector<VkQueueFamilyProperties>* queueFamilies, 
                                      const VkQueueFlags queueFlags) 
{
    for (const VkQueueFamilyProperties& queueFamily : *queueFamilies) {
        if ((queueFamily.queueFlags & queueFlags) == queueFlags) {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::UpdateQueueFamilyPropertiesMapping(const std::vector<VkQueueFlagBits>* requiredQueueFlags) {

    m_vulkanQueueFamilyIndexMap.clear();
    
    for (const VkQueueFlagBits& flag: *requiredQueueFlags) {

        const uint32_t numProperties = static_cast<uint32_t>(m_vulkanQueueFamilyProperties.size());
        for (uint32_t i = 0; i < numProperties; ++i) {
            const VkQueueFamilyProperties& curQueueFamily = m_vulkanQueueFamilyProperties[i];
            if (curQueueFamily.queueFlags & flag) {
                m_vulkanQueueFamilyIndexMap[flag] = i;
                break;
            }
        }
    }


}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CleanUp() {

    if (nullptr != m_vulkanLogicalDevice) {
        vkDestroyDevice(m_vulkanLogicalDevice, nullptr);
        m_vulkanLogicalDevice = nullptr;    
    }

    if (nullptr != m_vulkanInstance) {

        if (nullptr != m_vulkanSurface) {
            vkDestroySurfaceKHR(m_vulkanInstance, m_vulkanSurface, nullptr);
            m_vulkanSurface = nullptr;
        }
#ifdef ENABLE_VULKAN_DEBUG
        m_vulkanDebug.Shutdown(m_vulkanInstance);
#endif
        vkDestroyInstance(m_vulkanInstance, nullptr);
        m_vulkanInstance = nullptr;
    }

    if (nullptr != m_window) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = nullptr;
    }
   
}
