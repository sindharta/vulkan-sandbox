#include "TriangleApp.h"
#include <GLFW/glfw3.h> //GLFWwindow

#include <glm/gtc/matrix_transform.hpp> //glm::rotate, glm::lookAt, glm::perspective
#include <stdexcept> //std::runtime_error
#include <iostream> //cout
#include <set> 
#include <algorithm>  //max
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION    //This will include the implementation of STB, instead of only the header
#include "stb_image.h"  //for loading images


#include "Utilities/FileUtility.h"      //ReadFileInto()
#include "Utilities/GraphicsUtility.h"    //CreateShaderModule()    
#include "Utilities/Macros.h"

#include "Vertex/ColorVertex.h"    
#include "Vertex/TextureVertex.h"    
#include "MVPUniform.h"    

//Shared
#include "Window.h"


VkAllocationCallbacks* g_allocator = nullptr; //Always use default allocator

const std::vector<const char*> g_requiredVulkanLayers = {
    "VK_LAYER_KHRONOS_validation"
};

//There are two types of extensions: instance and device
const std::vector<const char*> g_requiredInstanceExtensions = {
#ifdef ENABLE_VULKAN_DEBUG
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
};

const std::vector<const char*> g_requiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<ColorVertex> g_colorVertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
};

const std::vector<TextureVertex> g_texVertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> g_indices = {
    0, 1, 2, 2, 3, 0
};

//---------------------------------------------------------------------------------------------------------------------
static void WindowResizedCallback(void* userData) {
    TriangleApp* app = reinterpret_cast<TriangleApp*>(userData);
    app->RequestToRecreateSwapChain();
}

//---------------------------------------------------------------------------------------------------------------------

TriangleApp::TriangleApp() 
    : m_instance(VK_NULL_HANDLE), m_surface(VK_NULL_HANDLE)
    , m_physicalDevice(VK_NULL_HANDLE), m_logicalDevice(VK_NULL_HANDLE)
    , m_graphicsQueue(VK_NULL_HANDLE)
    , m_swapChain(VK_NULL_HANDLE), m_renderPass(VK_NULL_HANDLE)
    , m_descriptorSetLayout(VK_NULL_HANDLE)
    , m_descriptorPool(VK_NULL_HANDLE)
    , m_pipelineLayout(VK_NULL_HANDLE), m_graphicsPipeline(VK_NULL_HANDLE)
    , m_commandPool(VK_NULL_HANDLE), m_currentFrame(0), m_recreateSwapChainRequested(false)
    , m_vb(VK_NULL_HANDLE), m_vbMemory(VK_NULL_HANDLE)
    , m_ib(VK_NULL_HANDLE), m_ibMemory(VK_NULL_HANDLE)
    , m_textureImage(VK_NULL_HANDLE), m_textureImageMemory(VK_NULL_HANDLE)
    , m_textureImageView(VK_NULL_HANDLE), m_textureSampler(VK_NULL_HANDLE)
    , m_window(nullptr) 
{
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::Run() {    
    m_window = new Window();
    m_window->Init(WIDTH,HEIGHT, WindowResizedCallback, this);

    PrintSupportedExtensions();
    InitVulkan();

#ifdef ENABLE_VULKAN_DEBUG
    if (VK_SUCCESS != m_Debug.Init(m_instance)) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
    
#endif //ENABLE_VULKAN_DEBUG

    Loop();
    CleanUp();
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

    m_window->CreateVulkanSurfaceInto(m_instance, g_allocator, &m_surface);
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateDescriptorSetLayout();
    CreateCommandPool();
    CreateTextureImage();
    CreateTextureImageView();
    CreateTextureSampler();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateSyncObjects();

    //Swap
    RecreateSwapChain();
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::RecreateSwapChain() {

    
    m_window->WaitInMinimizedState(); //Handle window minimization

    vkDeviceWaitIdle(m_logicalDevice);

    CleanUpVulkanSwapChain();
    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFrameBuffers();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
    m_recreateSwapChainRequested = false;

    m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::PickPhysicalDevice()  {
    //Pick physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const VkPhysicalDevice& device : devices) {
        //Check extension
        if (!CheckDeviceExtensionSupport(device, &g_requiredDeviceExtensions))
            continue;

        //Check features
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        if (!supportedFeatures.samplerAnisotropy)
            continue;

        //Check swap chain support
        PhysicalDeviceSurfaceInfo deviceSurfaceInfo = QueryVulkanPhysicalDeviceSurfaceInfo(device, m_surface);
        if (!deviceSurfaceInfo.IsSwapChainSupported())
            continue;

        //Check required queue family
        QueueFamilyIndices curIndices = QueryVulkanQueueFamilyIndices(device, m_surface);
        if (curIndices.IsComplete()) {
            m_physicalDevice = device;
            m_queueFamilyIndices = curIndices;
            break;
        }

    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateLogicalDevice()  {

    //Create device queue for all required queues
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { 
        m_queueFamilyIndices.GetGraphicsIndex(), 
        m_queueFamilyIndices.GetPresentIndex(),
    };
    const float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    //Device features
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    //Creating logical device
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(g_requiredDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = g_requiredDeviceExtensions.data();
    createInfo.enabledLayerCount =  0;

#ifdef ENABLE_VULKAN_DEBUG
    //[Note-sin: 2019-11-5] These settings are ignored by newer implementations of Vulkan
    createInfo.enabledLayerCount = static_cast<uint32_t>(g_requiredVulkanLayers.size());
    createInfo.ppEnabledLayerNames = g_requiredVulkanLayers.data();
#endif

    if (VK_SUCCESS !=vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice) ) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.GetGraphicsIndex(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.GetPresentIndex(), 0, &m_presentationQueue);
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::CreateDescriptorSetLayout() {
    //Uniform buffer
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //which shader stage will use this
    uboLayoutBinding.pImmutableSamplers = nullptr; 

    //Texture Sampler
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    //Bind
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_logicalDevice, &layoutInfo, g_allocator, &m_descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_queueFamilyIndices.GetGraphicsIndex();
    poolInfo.flags = 0; // Optional

    if (vkCreateCommandPool(m_logicalDevice, &poolInfo, g_allocator, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::CreateVertexBuffer() {

    const VkDeviceSize bufferSize = sizeof(g_texVertices[0]) * g_texVertices.size();


    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: to write from the CPU.
    //VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: ensure that the driver is aware of our copying. Alternative: use flush
    GraphicsUtility::CreateBuffer(m_physicalDevice, m_logicalDevice, g_allocator, bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        &stagingBuffer, &stagingBufferMemory);

    //Filling Vertex Buffer
    GraphicsUtility::CopyCPUDataToBuffer(m_logicalDevice,g_texVertices.data(),stagingBufferMemory,bufferSize);

    //VK_BUFFER_USAGE_TRANSFER_DST_BIT: destination in a memory transfer
    GraphicsUtility::CreateBuffer(m_physicalDevice, m_logicalDevice, g_allocator, bufferSize, 
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                 &m_vb, &m_vbMemory);

    //Copy buffer
    GraphicsUtility::CopyBuffer(m_logicalDevice, m_commandPool, m_graphicsQueue,
        stagingBuffer, m_vb,bufferSize
    );

    vkDestroyBuffer(m_logicalDevice, stagingBuffer, g_allocator);
    vkFreeMemory(m_logicalDevice, stagingBufferMemory, g_allocator);

}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateIndexBuffer() {
    const VkDeviceSize bufferSize = sizeof(g_indices[0]) * g_indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    GraphicsUtility::CreateBuffer(m_physicalDevice, m_logicalDevice, g_allocator, bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        &stagingBuffer, &stagingBufferMemory
    );

    GraphicsUtility::CopyCPUDataToBuffer(m_logicalDevice,g_indices.data(),stagingBufferMemory,bufferSize);

    GraphicsUtility::CreateBuffer(m_physicalDevice, m_logicalDevice,g_allocator, bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_ib, &m_ibMemory);

    //Copy buffer
    GraphicsUtility::CopyBuffer(m_logicalDevice, m_commandPool, m_graphicsQueue,
        stagingBuffer, m_ib,bufferSize
    );

    vkDestroyBuffer(m_logicalDevice, stagingBuffer, g_allocator);
    vkFreeMemory(m_logicalDevice, stagingBufferMemory, g_allocator);
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::CreateTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("../Resources/Textures/statue.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    const VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    GraphicsUtility::CreateBuffer(m_physicalDevice, m_logicalDevice, g_allocator, imageSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        &stagingBuffer, &stagingBufferMemory
    );

    GraphicsUtility::CopyCPUDataToBuffer(m_logicalDevice,pixels,stagingBufferMemory,imageSize);

    stbi_image_free(pixels);

    //Create Image buffer. 
    //VK_IMAGE_USAGE_SAMPLED_BIT: to allow access from the shader
    GraphicsUtility::CreateImage(m_physicalDevice,m_logicalDevice,g_allocator, texWidth, texHeight,
        VK_IMAGE_TILING_OPTIMAL,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,VK_FORMAT_R8G8B8A8_UNORM, &m_textureImage,&m_textureImageMemory
    );

    //Transition the texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    GraphicsUtility::DoImageLayoutTransition(m_logicalDevice, m_commandPool, m_graphicsQueue, 
        m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, 
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    //Execute the buffer to image copy operation
    GraphicsUtility::CopyBufferToImage(m_logicalDevice,m_commandPool, m_graphicsQueue,stagingBuffer, 
        m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    //one last transition to prepare it for shader access:
    GraphicsUtility::DoImageLayoutTransition(m_logicalDevice, m_commandPool, m_graphicsQueue, 
        m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    vkDestroyBuffer(m_logicalDevice, stagingBuffer, g_allocator);
    vkFreeMemory(m_logicalDevice, stagingBufferMemory, g_allocator);

}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateTextureImageView() {
    m_textureImageView = GraphicsUtility::CreateImageView(m_logicalDevice, 
        g_allocator, m_textureImage, VK_FORMAT_R8G8B8A8_UNORM);

}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::CreateTextureSampler() {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE; //[0..1] range
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    if (vkCreateSampler(m_logicalDevice, &samplerInfo, g_allocator, &m_textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::CreateUniformBuffers() {

    const VkDeviceSize bufferSize = sizeof(MVPUniform);
    const uint32_t numImages = static_cast<uint32_t>(m_swapChainImages.size());

    m_uniformBuffers.resize(numImages);
    m_uniformBuffersMemory.resize(numImages);

    //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: to write from the CPU.
    //VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: ensure that the driver is aware of our copying. Alternative: use flush
    for (uint32_t i = 0; i < numImages; ++i) {
        GraphicsUtility::CreateBuffer(m_physicalDevice, m_logicalDevice, g_allocator, bufferSize, 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            &m_uniformBuffers[i], &m_uniformBuffersMemory[i]
        );
    }
    
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::CreateCommandBuffers() {
    const uint32_t numFrameBuffers = static_cast<uint32_t>(m_swapChainFramebuffers.size());
    m_commandBuffers.resize(numFrameBuffers);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = numFrameBuffers;

    if (vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    //Starting command buffer recording
    for (size_t i = 0; i < numFrameBuffers; ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        //Starting a render pass
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_swapChainExtent;

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor; //to be used by VK_ATTACHMENT_LOAD_OP_CLEAR, when creating RenderPass
        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

        //Bind vertex and index buffers
        VkBuffer vertexBuffers[] = {m_vb};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(m_commandBuffers[i], m_ib, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, 
            m_pipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr
        );

        vkCmdDrawIndexed(m_commandBuffers[i], static_cast<uint32_t>(g_indices.size()), 1, 0, 0, 0);

        //
        vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, 
            m_pipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr
        );
        vkCmdDrawIndexed(m_commandBuffers[i], static_cast<uint32_t>(g_indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(m_commandBuffers[i]);
        if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateSyncObjects() {

    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //Don't make the GPU wait when rendering the first frame

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, g_allocator, &m_imageAvailableSemaphores[i]) != VK_SUCCESS 
            || vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, g_allocator, &m_renderFinishedSemaphores[i]) != VK_SUCCESS 
            || vkCreateFence(m_logicalDevice, &fenceInfo, g_allocator, &m_inFlightFences[i]) != VK_SUCCESS
        )
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateSwapChain() {
    //Decide parameters
    PhysicalDeviceSurfaceInfo surfaceInfo = QueryVulkanPhysicalDeviceSurfaceInfo(m_physicalDevice, m_surface);
    VkSurfaceCapabilitiesKHR& capabilities = surfaceInfo.Capabilities;

    const VkSurfaceFormatKHR surfaceFormat = PickSwapSurfaceFormat(&surfaceInfo.Formats);
    const VkPresentModeKHR presentMode = PickSwapPresentMode(&surfaceInfo.PresentModes);

    m_swapChainExtent = m_window->SelectVulkanSwapExtent(capabilities);
    
    uint32_t imageCount = capabilities.minImageCount + 1; //add +1 to prevent waiting for internal ops
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    //Create
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const uint32_t graphicsQueueFamilyIndex = m_queueFamilyIndices.GetGraphicsIndex();
    const uint32_t presentQueueFamilyIndex = m_queueFamilyIndices.GetPresentIndex();

    if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
        //Images can be used across multiple queue families without ownership transfers
        uint32_t queueFamilyIndices[] = { graphicsQueueFamilyIndex,  presentQueueFamilyIndex,  };
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        //An image is owned by one queue family at a time. Can transfer ownership
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_logicalDevice, &createInfo, g_allocator, &m_swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    m_swapChainSurfaceFormat = surfaceFormat.format;
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateImageViews() {
    //handles to swap chain images
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());
    std::cout << "Swap Chain Image Count: " << imageCount << std::endl;
    std::cout << "Max Frames in Flight: " << MAX_FRAMES_IN_FLIGHT << std::endl;


    uint32_t numImages = static_cast<uint32_t>(m_swapChainImages.size());
    m_swapChainImageViews.resize(numImages);

    for (size_t i = 0; i < numImages; i++) {
        m_swapChainImageViews[i] = GraphicsUtility::CreateImageView(m_logicalDevice, g_allocator, 
            m_swapChainImages[i], m_swapChainSurfaceFormat);

    }
}

//---------------------------------------------------------------------------------------------------------------------

//Renderpass is an orchestration of image data.  It helps the GPU better understand when wefll be drawing, 
//what we'll be drawing to, and what it should do between render passes.
void TriangleApp::CreateRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_swapChainSurfaceFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //No multisampling
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;    //No stencil
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  //No stencil
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  //So that after rendering, we can present

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0; //this is referred by shaders (layout location)
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;


    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_logicalDevice, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}


//---------------------------------------------------------------------------------------------------------------------

//A pool to create descriptor set to bind uniform buffers when drawing frame
void TriangleApp::CreateDescriptorPool() {

    const uint32_t numImages = static_cast<uint32_t>(m_swapChainImages.size());

    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(numImages);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(numImages);

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(numImages);

    if (vkCreateDescriptorPool(m_logicalDevice, &poolInfo, g_allocator, &m_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateDescriptorSets() {
    const uint32_t numImages = static_cast<uint32_t>(m_swapChainImages.size());

    std::vector<VkDescriptorSetLayout> layouts(numImages, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = numImages;
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(numImages);
    if (vkAllocateDescriptorSets(m_logicalDevice, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < numImages; ++i) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(MVPUniform);

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureImageView;
        imageInfo.sampler   = m_textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), 
            descriptorWrites.data(), 0, nullptr
        );
    }

}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateGraphicsPipeline() {
    #define SHADER_PATH "../Shared/Shaders/"
    std::vector<char> vertShaderCode, fragShaderCode;
    FileUtility::ReadFileInto(SHADER_PATH "Texture.vert.spv", &vertShaderCode);
    FileUtility::ReadFileInto(SHADER_PATH "Texture.frag.spv", &fragShaderCode);
    #undef SHADER_PATH

    VkShaderModule vertShaderModule = GraphicsUtility::CreateShaderModule(m_logicalDevice, g_allocator, vertShaderCode);
    VkShaderModule fragShaderModule = GraphicsUtility::CreateShaderModule(m_logicalDevice, g_allocator, fragShaderCode);

    //Vertex
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    vertShaderStageInfo.pSpecializationInfo = nullptr;//To specify shader constants

    //Frag
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    //Vertex Input
    const VkVertexInputBindingDescription*  bindingDescription = TextureVertex::GetBindingDescription();
    const std::vector<VkVertexInputAttributeDescription>* attributeDescriptions = TextureVertex::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions->size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescription; 
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions->data(); 

    //Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    //Viewport and Scissor 
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChainExtent.width);
    viewport.height = static_cast<float>(m_swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    //Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    //Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    //Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    //Dynamic states
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    //Pipeline layout: to pass uniform values to shaders
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; 
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout; 
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    
    if (vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutInfo, g_allocator, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    //Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, g_allocator, &m_graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(m_logicalDevice, fragShaderModule, g_allocator);
    vkDestroyShaderModule(m_logicalDevice, vertShaderModule, g_allocator);
}

//---------------------------------------------------------------------------------------------------------------------

//VkFrameBuffer is what maps the actual attachments (swap chain images) to a RenderPass. 
//The attachment definition was defined when creating the RenderPass
void TriangleApp::CreateFrameBuffers() {
    const uint32_t numImageViews = static_cast<uint32_t>(m_swapChainImageViews.size());
    m_swapChainFramebuffers.resize(numImageViews);
    

    for (size_t i = 0; i < numImageViews; i++) {
        VkImageView attachments[] = {
            m_swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_logicalDevice, &framebufferInfo, g_allocator, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
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
        = static_cast<const VulkanDebugMessenger>(m_Debug).GetCreateInfo();

    if (!CheckRequiredVulkanLayersAvailability(g_requiredVulkanLayers)) {
        throw std::runtime_error("Required Vulkan layers are requested, but some are not available!");
    }
    createInfo.enabledLayerCount = static_cast<uint32_t>(g_requiredVulkanLayers.size());
    createInfo.ppEnabledLayerNames = g_requiredVulkanLayers.data();

    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
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

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

#endif //ENABLE_VULKAN_DEBUG

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::Loop() {
    while (m_window->Loop()) {
        DrawFrame();
    }

    //Wait until all vulkan operations are finished
    vkDeviceWaitIdle(m_logicalDevice);
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::DrawFrame() {
    //The fence will sync CPU - GPU. Make sure that we are not processing the same frame in flight
    vkWaitForFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    //1. Acquire an image from the swap chain
    VkSemaphore curImageAvailableSemaphore = m_imageAvailableSemaphores[m_currentFrame];
    uint32_t imageIndex;
    {
        const VkResult result = vkAcquireNextImageKHR(m_logicalDevice, m_swapChain, UINT64_MAX, 
                                                      curImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
    }

    //Check if we are about to draw to an image from the swap chain that is still in flight.
    //This can happen for example if MAX_FRAMES_IN_FLIGHT >= the number of images in the swap chain.
    if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_logicalDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

    //Semaphores: GPU-GPU synchronization. No need to reset
    VkSemaphore waitSemaphores[] = {curImageAvailableSemaphore};
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};

    //2. Execute the command buffer with that image as attachment in the framebuffer
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    UpdateVulkanUniformBuffers(imageIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores; //Wait for the acquire process to finish
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    //Reset fence for syncing sync CPU - GPU
    vkResetFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame]);

    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    //3. Return the image to the swap chain for presentation. Wait for rendering to be finished
    VkPresentInfoKHR presentInfo = {};
    VkSwapchainKHR swapChains[] = {m_swapChain};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional
    {
        const VkResult result = vkQueuePresentKHR(m_presentationQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_recreateSwapChainRequested) {
            //The RecreateSwapChainRequested check is put here to make sure that the semaphores are in consistent state
            RecreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::UpdateVulkanUniformBuffers(uint32_t imageIndex) {
    static const auto START_TIME = std::chrono::high_resolution_clock::now();

    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - START_TIME).count();

    MVPUniform mvp = {};
    mvp.ModelMat = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mvp.ViewMat  = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mvp.ProjMat = glm::perspective(glm::radians(45.0f), m_swapChainExtent.width / static_cast<float> (m_swapChainExtent.height), 0.1f, 10.0f);
    mvp.ProjMat[1][1] *= -1; //flip Y axis

    GraphicsUtility::CopyCPUDataToBuffer(m_logicalDevice, &mvp, 
        m_uniformBuffersMemory[imageIndex],sizeof(mvp));
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
    extensions->insert(extensions->end(), g_requiredInstanceExtensions.begin(),g_requiredInstanceExtensions.end());

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

QueueFamilyIndices TriangleApp::QueryVulkanQueueFamilyIndices(const VkPhysicalDevice& device, 
                                                                const VkSurfaceKHR& surface) 
{
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    GetVulkanQueueFamilyPropertiesInto(device, &queueFamilyProperties);

    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = static_cast<uint32_t>(queueFamilyProperties.size());
    for (uint32_t i = 0; i < queueFamilyCount && !indices.IsComplete(); ++i) {
        const VkQueueFamilyProperties& curQueueFamily = queueFamilyProperties[i];
        if (!indices.IsGraphicsIndexSet() && (curQueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)  ) {
            indices.SetGraphicsIndex(i);
        }

        if (!indices.IsPresentIndexSet()) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.SetPresentIndex(i);
            }
        }

    }

    return indices;

}

//---------------------------------------------------------------------------------------------------------------------

PhysicalDeviceSurfaceInfo TriangleApp::QueryVulkanPhysicalDeviceSurfaceInfo(const VkPhysicalDevice& device, 
                                                           const VkSurfaceKHR& surface) 
{
    PhysicalDeviceSurfaceInfo details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
    }

    return details;
}

//---------------------------------------------------------------------------------------------------------------------
bool TriangleApp::CheckDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>* extensions) {

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    //if all required extensions exist in the available extensions, then the set will be empty.
    std::set<std::string> requiredExtensions(extensions->begin(), extensions->end());
    for (const VkExtensionProperties& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

//---------------------------------------------------------------------------------------------------------------------
VkSurfaceFormatKHR TriangleApp::PickSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* availableFormats) {

    for (const VkSurfaceFormatKHR& availableFormat : *availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    //If the ideal one is not found, then just return the first one
    if (availableFormats->size()>0)
        return availableFormats->at(0);

    return VkSurfaceFormatKHR();
}

//---------------------------------------------------------------------------------------------------------------------

VkPresentModeKHR TriangleApp::PickSwapPresentMode(const std::vector<VkPresentModeKHR>* availableModes) {
    for (const VkPresentModeKHR& availablePresentMode : *availableModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR; //guaranteed to be available
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CleanUp() {

    //Semaphores
    const uint32_t numSyncObjects = static_cast<uint32_t>(m_imageAvailableSemaphores.size());
    for (size_t i = 0; i < numSyncObjects; i++) {
        vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphores[i], g_allocator);
        vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphores[i], g_allocator);
        vkDestroyFence(m_logicalDevice, m_inFlightFences[i], g_allocator);
    }
    m_imageAvailableSemaphores.clear();
    m_renderFinishedSemaphores.clear();
    m_inFlightFences.clear();

    CleanUpVulkanSwapChain();
    
    SAFE_DESTROY_DESCRIPTOR_SET_LAYOUT(m_logicalDevice,m_descriptorSetLayout,g_allocator);

    //Textures
    SAFE_DESTROY_SAMPLER(m_logicalDevice,m_textureSampler,g_allocator);
    SAFE_DESTROY_IMAGE_VIEW(m_logicalDevice, m_textureImageView, g_allocator);
    SAFE_DESTROY_IMAGE(m_logicalDevice, m_textureImage, g_allocator);
    SAFE_FREE_MEMORY(m_logicalDevice, m_textureImageMemory, g_allocator);

    //Vertex and Index Buffers
    SAFE_DESTROY_BUFFER(m_logicalDevice, m_vb, g_allocator);
    SAFE_FREE_MEMORY(m_logicalDevice, m_vbMemory, g_allocator);
    SAFE_DESTROY_BUFFER(m_logicalDevice, m_ib, g_allocator);
    SAFE_FREE_MEMORY(m_logicalDevice, m_ibMemory, g_allocator);

    SAFE_DESTROY_COMMAND_POOL(m_logicalDevice, m_commandPool, g_allocator);

    m_graphicsQueue = VK_NULL_HANDLE;
    m_presentationQueue = VK_NULL_HANDLE;

    SAFE_DESTROY_DEVICE(m_logicalDevice, g_allocator);

    if (nullptr != m_instance) {

        if (nullptr != m_surface) {
            vkDestroySurfaceKHR(m_instance, m_surface, g_allocator);
            m_surface = VK_NULL_HANDLE;
        }
#ifdef ENABLE_VULKAN_DEBUG
        m_Debug.Shutdown(m_instance);
#endif
        vkDestroyInstance(m_instance, g_allocator);
        m_instance = VK_NULL_HANDLE;
    }

    if (nullptr != m_window) {
        m_window->CleanUp();
        delete m_window;
        m_window = nullptr;
    }
   
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CleanUpVulkanSwapChain() {

    const uint32_t numImages = static_cast<uint32_t>(m_swapChainImages.size());
    vkFreeCommandBuffers(m_logicalDevice, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

    //Uniform buffers
    for (size_t i = 0; i < numImages; i++) {
        vkDestroyBuffer(m_logicalDevice, m_uniformBuffers[i], g_allocator);
        vkFreeMemory(m_logicalDevice, m_uniformBuffersMemory[i], g_allocator);
    }
    m_uniformBuffers.clear();
    m_uniformBuffersMemory.clear();

    SAFE_DESTROY_DESCRIPTOR_POOL(m_logicalDevice, m_descriptorPool, g_allocator);

    for (VkFramebuffer& framebuffer : m_swapChainFramebuffers) {
        vkDestroyFramebuffer(m_logicalDevice, framebuffer, g_allocator);
    }
    m_swapChainFramebuffers.clear();

    for (VkImageView& imageView : m_swapChainImageViews) {
        vkDestroyImageView(m_logicalDevice, imageView, g_allocator);
    }
    m_swapChainImages.clear();

    SAFE_DESTROY_PIPELINE(m_logicalDevice, m_graphicsPipeline, g_allocator);
    SAFE_DESTROY_PIPELINE_LAYOUT(m_logicalDevice, m_pipelineLayout, g_allocator);
    SAFE_DESTROY_RENDER_PASS(m_logicalDevice, m_renderPass, g_allocator);
    SAFE_DESTROY_SWAP_CHAIN(m_logicalDevice, m_swapChain, g_allocator);
}
