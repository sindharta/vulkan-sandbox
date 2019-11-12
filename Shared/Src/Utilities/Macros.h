#pragma once

#define SAFE_DESTROY_DESCRIPTOR_POOL(device, obj, allocator) { \
    if (VK_NULL_HANDLE!=obj) { \
        vkDestroyDescriptorPool(device, obj, g_allocator); \
        obj = VK_NULL_HANDLE; \
    } \
}

#define SAFE_DESTROY_PIPELINE(device, obj, allocator) { \
    if (VK_NULL_HANDLE != obj) { \
        vkDestroyPipeline(device, obj, allocator); \
        obj = VK_NULL_HANDLE; \
    } \
}

#define SAFE_DESTROY_PIPELINE_LAYOUT(device, obj, allocator) { \
    if (VK_NULL_HANDLE != obj) { \
        vkDestroyPipelineLayout(device, obj, allocator); \
        obj = VK_NULL_HANDLE; \
    } \
}

#define SAFE_DESTROY_RENDER_PASS(device, obj, allocator) { \
    if (VK_NULL_HANDLE != obj) { \
        vkDestroyRenderPass(device, obj, allocator); \
        obj = VK_NULL_HANDLE;\
    } \
}

#define SAFE_DESTROY_SWAP_CHAIN(device, obj, allocator) { \
    if (VK_NULL_HANDLE!= obj) { \
        vkDestroySwapchainKHR(device, obj, g_allocator); \
        obj = VK_NULL_HANDLE; \
    } \
}

#define SAFE_DESTROY_DESCRIPTOR_SET_LAYOUT(device, obj, allocator) { \
    if (VK_NULL_HANDLE != obj) { \
        vkDestroyDescriptorSetLayout(device, obj, allocator); \
        obj = VK_NULL_HANDLE; \
    } \
}

#define SAFE_DESTROY_SAMPLER(device, obj, allocator) { \
    if (VK_NULL_HANDLE != obj) { \
        vkDestroySampler(device, obj, allocator); \
        obj = VK_NULL_HANDLE; \
    } \
}

#define SAFE_DESTROY_IMAGE_VIEW(device, obj, allocator) { \
    if (VK_NULL_HANDLE != obj) { \
        vkDestroyImageView(device, obj, allocator); \
        obj = VK_NULL_HANDLE; \
    } \
}

#define SAFE_DESTROY_IMAGE(device, obj, allocator) { \
    if (VK_NULL_HANDLE!=obj) { \
        vkDestroyImage(device, obj, allocator); \
        obj = VK_NULL_HANDLE; \
    } \
}

#define SAFE_FREE_MEMORY(device, obj, allocator) { \
    if (VK_NULL_HANDLE!=obj) { \
        vkFreeMemory(device, obj, allocator); \
        obj = VK_NULL_HANDLE; \
    } \
}

#define SAFE_DESTROY_BUFFER(device, obj, allocator) { \
    if (VK_NULL_HANDLE != obj) { \
        vkDestroyBuffer(device, obj, allocator); \
        obj = VK_NULL_HANDLE; \
    } \
}

#define SAFE_DESTROY_COMMAND_POOL(device, obj, allocator) { \
    if (VK_NULL_HANDLE != obj) { \
        vkDestroyCommandPool(device, obj, allocator); \
        obj = VK_NULL_HANDLE; \
    } \
}

#define SAFE_DESTROY_DEVICE(device, allocator) { \
    if (VK_NULL_HANDLE != device) { \
        vkDestroyDevice(device, allocator); \
        device = VK_NULL_HANDLE; \
    } \
}


