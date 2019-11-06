#pragma once
#include <vulkan/vulkan.h> 
#include <vector>

struct PhysicalDeviceSurfaceInfo {
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR> PresentModes;

    inline bool IsSwapChainSupported(); 

};

//---------------------------------------------------------------------------------------------------------------------

bool PhysicalDeviceSurfaceInfo::IsSwapChainSupported() { return !Formats.empty() && !PresentModes.empty(); }
