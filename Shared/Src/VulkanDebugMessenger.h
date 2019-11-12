#pragma once

#include "SharedConfig.h"
#include <vulkan/vulkan.h> 

#ifdef ENABLE_VULKAN_DEBUG

class VulkanDebugMessenger {
public:
    VulkanDebugMessenger();
    VkResult Init(VkInstance& vulkan);
    void Shutdown(VkInstance& vulkan);
    inline const VkDebugUtilsMessengerCreateInfoEXT& GetCreateInfo() const;


private:
    inline VkDebugUtilsMessengerCreateInfoEXT& GetCreateInfo();
    VkDebugUtilsMessengerEXT  m_messenger;
    VkDebugUtilsMessengerCreateInfoEXT m_createInfo;
};

//---------------------------------------------------------------------------------------------------------------------

const VkDebugUtilsMessengerCreateInfoEXT& VulkanDebugMessenger::GetCreateInfo() const { return m_createInfo; }


#endif //ENABLE_VULKAN_DEBUG
