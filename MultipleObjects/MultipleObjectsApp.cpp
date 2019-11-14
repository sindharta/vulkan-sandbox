#include "MultipleObjectsApp.h"
#include <GLFW/glfw3.h> //glfwGetRequiredInstanceExtensions

#include <stdexcept> //std::runtime_error
#include <iostream> //cout
#include <set> 
#include <array> 
#include <chrono>


//Shared
#include "Shin/Window.h"
#include "Shin/Utilities/GraphicsUtility.h"    //CreateImageView()    
#include "Shin/Utilities/Macros.h"
#include "Shin/Vertex/ColorVertex.h"    
#include "Shin/Vertex/TextureVertex.h"    
#include "Shin/Mesh.h"
#include "Shin/Texture.h"
#include "Shin/DrawPipeline.h"

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
    MultipleObjectsApp* app = reinterpret_cast<MultipleObjectsApp*>(userData);
    app->RequestToRecreateSwapChain();
}

//---------------------------------------------------------------------------------------------------------------------

MultipleObjectsApp::MultipleObjectsApp() 
    : m_instance(VK_NULL_HANDLE), m_surface(VK_NULL_HANDLE)
    , m_physicalDevice(VK_NULL_HANDLE), m_logicalDevice(VK_NULL_HANDLE)
    , m_graphicsQueue(VK_NULL_HANDLE)
    , m_swapChain(VK_NULL_HANDLE), m_renderPass(VK_NULL_HANDLE)
    , m_descriptorPool(VK_NULL_HANDLE)
    , m_colorDescriptorSetLayout(VK_NULL_HANDLE)
    , m_texDescriptorSetLayout(VK_NULL_HANDLE)
    , m_commandPool(VK_NULL_HANDLE), m_currentFrame(0), m_recreateSwapChainRequested(false)
    , m_texMesh(nullptr), m_colorMesh(nullptr)
    , m_texture(nullptr)
    , m_window(nullptr) 
{
}

//---------------------------------------------------------------------------------------------------------------------

void MultipleObjectsApp::Run() {    
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
void MultipleObjectsApp::InitVulkan() {

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

    //Init texture
    m_texture = new Shin::Texture();
    m_texture->Init(m_physicalDevice, m_logicalDevice, g_allocator, m_commandPool,m_graphicsQueue, 
        "../Resources/Textures/statue.jpg"
    );

    //Model
    const uint32_t numIndices = static_cast<uint32_t>(g_indices.size());
    m_texMesh = new Shin::Mesh();
    m_texMesh->Init(m_physicalDevice, m_logicalDevice, g_allocator, m_commandPool,m_graphicsQueue, 
        reinterpret_cast<const char*>(g_texVertices.data()), static_cast<uint32_t>(sizeof(g_texVertices[0]) * g_texVertices.size()),
        reinterpret_cast<const char*>(g_indices.data()), static_cast<uint32_t>(sizeof(g_indices[0]) * numIndices),
        numIndices
    );
    m_colorMesh = new Shin::Mesh();
    m_colorMesh->Init(m_physicalDevice, m_logicalDevice, g_allocator, m_commandPool,m_graphicsQueue, 
        reinterpret_cast<const char*>(g_colorVertices.data()), static_cast<uint32_t>(sizeof(g_colorVertices[0]) * g_colorVertices.size()),
        reinterpret_cast<const char*>(g_indices.data()), static_cast<uint32_t>(sizeof(g_indices[0]) * numIndices),
        numIndices
    );


    CreateSyncObjects();

    const uint32_t NUM_DRAW_OBJECTS = 5;
    const uint32_t NUM_DRAW_PIPELINES    = 2;

    //Init drawObjects
    m_drawObjects.resize(NUM_DRAW_OBJECTS);
    m_drawObjects[0].Init(m_logicalDevice, g_allocator, m_texMesh, m_texture);
    m_drawObjects[1].Init(m_logicalDevice, g_allocator, m_colorMesh, static_cast<Shin::Texture*>(nullptr));
    m_drawObjects[2].Init(m_logicalDevice, g_allocator, m_texMesh, m_texture);
    m_drawObjects[3].Init(m_logicalDevice, g_allocator, m_colorMesh, static_cast<Shin::Texture*>(nullptr));
    m_drawObjects[4].Init(m_logicalDevice, g_allocator, m_texMesh, m_texture);

    for (uint32_t i=0;i<NUM_DRAW_OBJECTS;++i) {
        m_drawObjects[i].SetPos(0,0,-1.0f + (0.5f * i));
    }

    //Init pipelines
    m_drawPipelines.resize(NUM_DRAW_PIPELINES);
    for (uint32_t i = 0; i < NUM_DRAW_PIPELINES; ++i) {
        m_drawPipelines[i] = new Shin::DrawPipeline();
    }

    #define SHADER_PATH "../Shared/Shaders/"

    m_drawPipelines[0]->Init(m_logicalDevice, g_allocator,
        SHADER_PATH "Texture.vert.spv",
        SHADER_PATH "Texture.frag.spv", 
        TextureVertex::GetBindingDescription(),
        TextureVertex::GetAttributeDescriptions(),
        m_texDescriptorSetLayout
    );
    m_drawPipelines[1]->Init(m_logicalDevice, g_allocator,
        SHADER_PATH "Color.vert.spv",
        SHADER_PATH "Color.frag.spv", 
        ColorVertex::GetBindingDescription(),
        ColorVertex::GetAttributeDescriptions(),
        m_colorDescriptorSetLayout
    );
    #undef SHADER_PATH

    m_drawPipelines[0]->AddDrawObject(&m_drawObjects[0]);
    m_drawPipelines[1]->AddDrawObject(&m_drawObjects[1]);
    m_drawPipelines[0]->AddDrawObject(&m_drawObjects[2]);
    m_drawPipelines[1]->AddDrawObject(&m_drawObjects[3]);
    m_drawPipelines[0]->AddDrawObject(&m_drawObjects[4]);

    //Swap
    RecreateSwapChain();
}

//---------------------------------------------------------------------------------------------------------------------

void MultipleObjectsApp::RecreateSwapChain() {
    
    m_window->WaitInMinimizedState(); //Handle window minimization

    vkDeviceWaitIdle(m_logicalDevice);

    CleanUpVulkanSwapChain();
    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();

    CreateFrameBuffers();
    CreateDescriptorPool();

    //Recreate pipeline
    const uint32_t numImages = static_cast<uint32_t>(m_swapChainImages.size());
    const uint32_t numPipelines = static_cast<uint32_t>(m_drawPipelines.size());
    for (uint32_t i = 0; i < numPipelines; ++i) {
        m_drawPipelines[i]->RecreateSwapChainObjects(m_physicalDevice, m_logicalDevice, g_allocator, 
            m_descriptorPool, numImages, m_renderPass, m_swapChainExtent            
        );
    }

    CreateCommandBuffers();
    m_recreateSwapChainRequested = false;

    m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

}

//---------------------------------------------------------------------------------------------------------------------

void MultipleObjectsApp::PickPhysicalDevice()  {
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

void MultipleObjectsApp::CreateLogicalDevice()  {

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

void MultipleObjectsApp::CreateDescriptorSetLayout() {
    //Uniform buffer
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //which shader stage will use this
    uboLayoutBinding.pImmutableSamplers = nullptr; 

    {
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

        if (vkCreateDescriptorSetLayout(m_logicalDevice, &layoutInfo, g_allocator, &m_texDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    {
        std::array<VkDescriptorSetLayoutBinding, 1> bindings = {uboLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        if (vkCreateDescriptorSetLayout(m_logicalDevice, &layoutInfo, g_allocator, &m_colorDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

    }

}

//---------------------------------------------------------------------------------------------------------------------

void MultipleObjectsApp::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_queueFamilyIndices.GetGraphicsIndex();
    poolInfo.flags = 0; // Optional

    if (vkCreateCommandPool(m_logicalDevice, &poolInfo, g_allocator, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MultipleObjectsApp::CreateCommandBuffers() {
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

        const uint32_t numPipelines = static_cast<uint32_t>(m_drawPipelines.size());
        for (uint32_t j = 0; j < numPipelines; ++j) {
            m_drawPipelines[j]->Bind(m_commandBuffers[i]);
            m_drawPipelines[j]->DrawToCommandBuffer(m_commandBuffers[i], static_cast<uint32_t>(i));
        }

        vkCmdEndRenderPass(m_commandBuffers[i]);
        if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void MultipleObjectsApp::CreateSyncObjects() {

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

void MultipleObjectsApp::CreateSwapChain() {
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

void MultipleObjectsApp::CreateImageViews() {
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
void MultipleObjectsApp::CreateRenderPass() {
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
void MultipleObjectsApp::CreateDescriptorPool() {

    const uint32_t numImages = static_cast<uint32_t>(m_swapChainImages.size());
    const uint32_t maxDescriptorCount = static_cast<uint32_t>(m_drawObjects.size()) * numImages;

    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = maxDescriptorCount;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = maxDescriptorCount;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxDescriptorCount;

    if (vkCreateDescriptorPool(m_logicalDevice, &poolInfo, g_allocator, &m_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}


//---------------------------------------------------------------------------------------------------------------------

//VkFrameBuffer is what maps the actual attachments (swap chain images) to a RenderPass. 
//The attachment definition was defined when creating the RenderPass
void MultipleObjectsApp::CreateFrameBuffers() {
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
void MultipleObjectsApp::InitVulkanDebugInstance(const VkApplicationInfo& appInfo, const std::vector<const char*>& extensions) 
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

bool MultipleObjectsApp::CheckRequiredVulkanLayersAvailability(const std::vector<const char*> requiredLayers) {
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
void MultipleObjectsApp::InitVulkanInstance(const VkApplicationInfo& appInfo, const std::vector<const char*>& extensions) {


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

void MultipleObjectsApp::Loop() {
    while (m_window->Loop()) {
        DrawFrame();
    }

    //Wait until all vulkan operations are finished
    vkDeviceWaitIdle(m_logicalDevice);
}

//---------------------------------------------------------------------------------------------------------------------

void MultipleObjectsApp::DrawFrame() {
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
void MultipleObjectsApp::UpdateVulkanUniformBuffers(uint32_t imageIndex) {

    static const auto START_TIME = std::chrono::high_resolution_clock::now();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - START_TIME).count();

    const uint32_t numObjects = static_cast<uint32_t>(m_drawObjects.size());
    for (uint32_t i = 0; i < numObjects; ++i) {
        m_drawObjects[i].Rotate(time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        m_drawObjects[i].UpdateUniformBuffers(m_logicalDevice, imageIndex);
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MultipleObjectsApp::PrintSupportedExtensions() {
    
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

void MultipleObjectsApp::GetRequiredExtensionsInto(std::vector<const char*>* extensions) {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    extensions->insert(extensions->end(), &glfwExtensions[0], &glfwExtensions[glfwExtensionCount]);   
    extensions->insert(extensions->end(), g_requiredInstanceExtensions.begin(),g_requiredInstanceExtensions.end());

}

//---------------------------------------------------------------------------------------------------------------------
void MultipleObjectsApp::GetVulkanQueueFamilyPropertiesInto(const VkPhysicalDevice& device, 
                                                     std::vector<VkQueueFamilyProperties>* queueFamilies) 
{
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    queueFamilies->resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies->data());
}

//---------------------------------------------------------------------------------------------------------------------

QueueFamilyIndices MultipleObjectsApp::QueryVulkanQueueFamilyIndices(const VkPhysicalDevice& device, 
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

PhysicalDeviceSurfaceInfo MultipleObjectsApp::QueryVulkanPhysicalDeviceSurfaceInfo(const VkPhysicalDevice& device, 
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
bool MultipleObjectsApp::CheckDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>* extensions) {

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
VkSurfaceFormatKHR MultipleObjectsApp::PickSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* availableFormats) {

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

VkPresentModeKHR MultipleObjectsApp::PickSwapPresentMode(const std::vector<VkPresentModeKHR>* availableModes) {
    for (const VkPresentModeKHR& availablePresentMode : *availableModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR; //guaranteed to be available
}

//---------------------------------------------------------------------------------------------------------------------

void MultipleObjectsApp::CleanUp() {

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

    SAFE_DESTROY_DESCRIPTOR_SET_LAYOUT(m_logicalDevice,m_texDescriptorSetLayout,g_allocator);
    SAFE_DESTROY_DESCRIPTOR_SET_LAYOUT(m_logicalDevice,m_colorDescriptorSetLayout,g_allocator);

    //Draw Pipelines
    const uint32_t numDrawPipelines = static_cast<uint32_t>(m_drawPipelines.size());
    for (uint32_t i = 0; i < numDrawPipelines; ++i) {
        m_drawPipelines[i]->CleanUp(m_logicalDevice, g_allocator);
        delete(m_drawPipelines[i]);
    }
    m_drawPipelines.clear();

    //Draw Objects
    const uint32_t numDrawObjects = static_cast<uint32_t>(m_drawObjects.size());
    for (uint32_t i = 0; i < numDrawObjects; ++i) {
        m_drawObjects[i].CleanUp(m_logicalDevice, g_allocator);
    }
    m_drawObjects.clear();

    //Textures
    SAFE_CLEANUP_PTR(m_logicalDevice, g_allocator, m_texture);

    //Model
    SAFE_CLEANUP_PTR(m_logicalDevice, g_allocator, m_texMesh);
    SAFE_CLEANUP_PTR(m_logicalDevice, g_allocator, m_colorMesh);


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

void MultipleObjectsApp::CleanUpVulkanSwapChain() {

    const uint32_t numImages = static_cast<uint32_t>(m_swapChainImages.size());
    vkFreeCommandBuffers(m_logicalDevice, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

    const uint32_t numDrawPipelines = static_cast<uint32_t>(m_drawPipelines.size());
    for (uint32_t i = 0; i < numDrawPipelines; ++i) {
        m_drawPipelines[i]->CleanUpSwapChainObjects(m_logicalDevice, g_allocator);
    }
    
    SAFE_DESTROY_DESCRIPTOR_POOL(m_logicalDevice, m_descriptorPool, g_allocator);

    for (VkFramebuffer& framebuffer : m_swapChainFramebuffers) {
        vkDestroyFramebuffer(m_logicalDevice, framebuffer, g_allocator);
    }
    m_swapChainFramebuffers.clear();

    for (VkImageView& imageView : m_swapChainImageViews) {
        vkDestroyImageView(m_logicalDevice, imageView, g_allocator);
    }
    m_swapChainImages.clear();



    SAFE_DESTROY_RENDER_PASS(m_logicalDevice, m_renderPass, g_allocator);
    SAFE_DESTROY_SWAP_CHAIN(m_logicalDevice, m_swapChain, g_allocator);
}
