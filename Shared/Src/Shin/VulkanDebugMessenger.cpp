#include "VulkanDebugMessenger.h"
#include <iostream> //cerr

#ifdef ENABLE_VULKAN_DEBUG

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugLogCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) 
{

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

//---------------------------------------------------------------------------------------------------------------------

VulkanDebugMessenger::VulkanDebugMessenger() : m_messenger(nullptr){
    m_createInfo = {};
    m_createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    m_createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    m_createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    m_createInfo.pfnUserCallback = DebugLogCallback;
    m_createInfo.pUserData = nullptr; // Optional
}

//---------------------------------------------------------------------------------------------------------------------

VkResult VulkanDebugMessenger::Init(VkInstance& vulkan) {

    PFN_vkCreateDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(vulkan, "vkCreateDebugUtilsMessengerEXT")
    );
    if (nullptr!=func) {
        return func(vulkan, &m_createInfo, nullptr, &m_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

//---------------------------------------------------------------------------------------------------------------------

void VulkanDebugMessenger::Shutdown(VkInstance& vulkan) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(vulkan, "vkDestroyDebugUtilsMessengerEXT")
    );
    if (nullptr!=m_messenger && func != nullptr ) {
        func(vulkan, m_messenger, nullptr);
    }
}

#endif //#ifdef ENABLE_VULKAN_DEBUG
