#include "TriangleApp.h"
#include <GLFW/glfw3.h>
#include <stdexcept> //std::runtime_error
#include <iostream> //cout
#include <set> 
#include <algorithm>  //max

#include "Utilities/FileUtility.h"      //ReadFileInto()
#include "Utilities/GraphicsUtility.h"    //CreateShaderModule()    
#include "ColorVertex.h"    

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

const std::vector<ColorVertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

//---------------------------------------------------------------------------------------------------------------------
static void WindowResizedCallback(GLFWwindow* window, int width, int height) {
    TriangleApp* app = reinterpret_cast<TriangleApp*>(glfwGetWindowUserPointer(window));
    app->RequestToRecreateSwapChain();
}

//---------------------------------------------------------------------------------------------------------------------

TriangleApp::TriangleApp() 
    : m_vulkanInstance(nullptr), m_vulkanSurface(nullptr)
    , m_vulkanPhysicalDevice(VK_NULL_HANDLE), m_vulkanLogicalDevice(nullptr), m_vulkanGraphicsQueue(nullptr)
    , m_vulkanSwapChain(nullptr), m_vulkanRenderPass(nullptr), m_vulkanPipelineLayout(nullptr), m_vulkanGraphicsPipeline(nullptr)
    , m_vulkanCommandPool(nullptr), m_vulkanCurrentFrame(0), m_recreateSwapChainRequested(false)
    , m_vulkanVB(nullptr), m_vulkanVBMemory(nullptr)
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
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan window", nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, WindowResizedCallback);
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

    CreateVulkanSurface();
    PickVulkanPhysicalDevice();
    CreateVulkanLogicalDevice();
    CreateVulkanSwapChain();
    CreateVulkanImageViews();
    CreateVulkanRenderPass();
    CreateVulkanGraphicsPipeline();
    CreateVulkanFrameBuffers();
    CreateVulkanCommandPool();
    CreateVulkanVertexBuffers();
    CreateVulkanCommandBuffers();
    CreateVulkanSyncObjects();
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::RecreateVulkanSwapChain() {

    //Handle window minimization
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_vulkanLogicalDevice);

    CleanUpVulkanSwapChain();
    CreateVulkanSwapChain();
    CreateVulkanImageViews();
    CreateVulkanRenderPass();
    CreateVulkanGraphicsPipeline();
    CreateVulkanFrameBuffers();
    CreateVulkanCommandBuffers();
    m_recreateSwapChainRequested = false;

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
        //Check extension
        if (!CheckDeviceExtensionSupport(device, &g_requiredDeviceExtensions))
            continue;

        //Check swap chain support
        PhysicalDeviceSurfaceInfo deviceSurfaceInfo = QueryVulkanPhysicalDeviceSurfaceInfo(device, m_vulkanSurface);
        if (!deviceSurfaceInfo.IsSwapChainSupported())
            continue;

        //Check required queue family
        QueueFamilyIndices curIndices = QueryVulkanQueueFamilyIndices(device, m_vulkanSurface);
        if (curIndices.IsComplete()) {
            m_vulkanPhysicalDevice = device;
            m_vulkanQueueFamilyIndices = curIndices;
            break;
        }
    }

    if (m_vulkanPhysicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateVulkanLogicalDevice()  {

    //Create device queue for all required queues
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { 
        m_vulkanQueueFamilyIndices.GetGraphicsIndex(), 
        m_vulkanQueueFamilyIndices.GetPresentIndex(),
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

    if (VK_SUCCESS !=vkCreateDevice(m_vulkanPhysicalDevice, &createInfo, nullptr, &m_vulkanLogicalDevice) ) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(m_vulkanLogicalDevice, m_vulkanQueueFamilyIndices.GetGraphicsIndex(), 0, &m_vulkanGraphicsQueue);
    vkGetDeviceQueue(m_vulkanLogicalDevice, m_vulkanQueueFamilyIndices.GetPresentIndex(), 0, &m_vulkanPresentationQueue);
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateVulkanSwapChain() {
    //Decide parameters
    PhysicalDeviceSurfaceInfo surfaceInfo = QueryVulkanPhysicalDeviceSurfaceInfo(m_vulkanPhysicalDevice, m_vulkanSurface);
    VkSurfaceCapabilitiesKHR& capabilities = surfaceInfo.Capabilities;

    VkSurfaceFormatKHR surfaceFormat = PickVulkanSwapSurfaceFormat(&surfaceInfo.Formats);
    VkPresentModeKHR presentMode = PickVulkanSwapPresentMode(&surfaceInfo.PresentModes);
    m_vulkanSwapChainExtent = PickVulkanSwapExtent(m_window, capabilities);
    
    uint32_t imageCount = capabilities.minImageCount + 1; //add +1 to prevent waiting for internal ops
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    //Create
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_vulkanSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_vulkanSwapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const uint32_t graphicsQueueFamilyIndex = m_vulkanQueueFamilyIndices.GetGraphicsIndex();
    const uint32_t presentQueueFamilyIndex = m_vulkanQueueFamilyIndices.GetPresentIndex();

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

    if (vkCreateSwapchainKHR(m_vulkanLogicalDevice, &createInfo, g_allocator, &m_vulkanSwapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    m_vulkanSwapChainSurfaceFormat = surfaceFormat.format;
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateVulkanImageViews() {
    //handles to swap chain images
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(m_vulkanLogicalDevice, m_vulkanSwapChain, &imageCount, nullptr);
    m_vulkanSwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_vulkanLogicalDevice, m_vulkanSwapChain, &imageCount, m_vulkanSwapChainImages.data());
    std::cout << "Swap Chain Image Count: " << imageCount << std::endl;
    std::cout << "Max Frames in Flight: " << MAX_FRAMES_IN_FLIGHT << std::endl;


    uint32_t numImages = static_cast<uint32_t>(m_vulkanSwapChainImages.size());
    m_vulkanSwapChainImageViews.resize(numImages);

    for (size_t i = 0; i < numImages; i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_vulkanSwapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_vulkanSwapChainSurfaceFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_vulkanLogicalDevice, &createInfo, g_allocator, &m_vulkanSwapChainImageViews[i]) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

//Renderpass is an orchestration of image data.  It helps the GPU better understand when wefll be drawing, 
//what we'll be drawing to, and what it should do between render passes.
void TriangleApp::CreateVulkanRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_vulkanSwapChainSurfaceFormat;
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

    if (vkCreateRenderPass(m_vulkanLogicalDevice, &renderPassInfo, nullptr, &m_vulkanRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateVulkanGraphicsPipeline() {
    //[TODO-sin: 2019-11-6] Add a post build step to compile source code to bytecode 
    std::vector<char> vertShaderCode, fragShaderCode;
    FileUtility::ReadFileInto("Shaders/Triangle.vert.spv", &vertShaderCode);
    FileUtility::ReadFileInto("Shaders/Triangle.frag.spv", &fragShaderCode);


    VkShaderModule vertShaderModule = GraphicsUtility::CreateShaderModule(m_vulkanLogicalDevice, g_allocator, vertShaderCode);
    VkShaderModule fragShaderModule = GraphicsUtility::CreateShaderModule(m_vulkanLogicalDevice, g_allocator, fragShaderCode);

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
    VkVertexInputBindingDescription  bindingDescription = ColorVertex::GetBindingDescription();
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = ColorVertex::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; 
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); 

    //Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    //Viewport and Scissor 
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_vulkanSwapChainExtent.width);
    viewport.height = static_cast<float>(m_vulkanSwapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_vulkanSwapChainExtent;

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
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(m_vulkanLogicalDevice, &pipelineLayoutInfo, g_allocator, &m_vulkanPipelineLayout) != VK_SUCCESS) {
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
    pipelineInfo.layout = m_vulkanPipelineLayout;
    pipelineInfo.renderPass = m_vulkanRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(m_vulkanLogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, g_allocator, &m_vulkanGraphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(m_vulkanLogicalDevice, fragShaderModule, g_allocator);
    vkDestroyShaderModule(m_vulkanLogicalDevice, vertShaderModule, g_allocator);
}

//---------------------------------------------------------------------------------------------------------------------

//VkFrameBuffer is what maps the actual attachments (swap chain images) to a RenderPass. 
//The attachment definition was defined when creating the RenderPass
void TriangleApp::CreateVulkanFrameBuffers() {
    const uint32_t numImageViews = static_cast<uint32_t>(m_vulkanSwapChainImageViews.size());
    m_vulkanSwapChainFramebuffers.resize(numImageViews);
    

    for (size_t i = 0; i < numImageViews; i++) {
        VkImageView attachments[] = {
            m_vulkanSwapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_vulkanRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_vulkanSwapChainExtent.width;
        framebufferInfo.height = m_vulkanSwapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_vulkanLogicalDevice, &framebufferInfo, g_allocator, &m_vulkanSwapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateVulkanCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_vulkanQueueFamilyIndices.GetGraphicsIndex();
    poolInfo.flags = 0; // Optional

    if (vkCreateCommandPool(m_vulkanLogicalDevice, &poolInfo, g_allocator, &m_vulkanCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::CreateVulkanVertexBuffers() {

    const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: to write from the CPU.
    //VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: ensure that the driver is aware of our copying. Alternative: use flush
    GraphicsUtility::CreateBuffer(m_vulkanPhysicalDevice, m_vulkanLogicalDevice, bufferSize, 
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        &m_vulkanVB, &m_vulkanVBMemory);

    //Filling Vertex Buffer
    void* data = nullptr;
    vkMapMemory(m_vulkanLogicalDevice, m_vulkanVBMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), bufferSize);
    vkUnmapMemory(m_vulkanLogicalDevice, m_vulkanVBMemory);

}


//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateVulkanCommandBuffers() {
    uint32_t numFrameBuffers = static_cast<uint32_t>(m_vulkanSwapChainFramebuffers.size());
    m_vulkanCommandBuffers.resize(numFrameBuffers);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_vulkanCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = numFrameBuffers;

    if (vkAllocateCommandBuffers(m_vulkanLogicalDevice, &allocInfo, m_vulkanCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    //Starting command buffer recording
    for (size_t i = 0; i < numFrameBuffers; ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(m_vulkanCommandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

       //Starting a render pass
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_vulkanRenderPass;
        renderPassInfo.framebuffer = m_vulkanSwapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_vulkanSwapChainExtent;

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor; //to be used by VK_ATTACHMENT_LOAD_OP_CLEAR, when creating RenderPass
        vkCmdBeginRenderPass(m_vulkanCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(m_vulkanCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipeline);

        //Bind vertex buffers
        VkBuffer vertexBuffers[] = {m_vulkanVB};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(m_vulkanCommandBuffers[i], 0, 1, vertexBuffers, offsets);

        vkCmdDraw(m_vulkanCommandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
        vkCmdEndRenderPass(m_vulkanCommandBuffers[i]);
        if (vkEndCommandBuffer(m_vulkanCommandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CreateVulkanSyncObjects() {

    m_vulkanImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_vulkanRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_vulkanInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_vulkanImagesInFlight.resize(m_vulkanSwapChainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //Don't make the GPU wait when rendering the first frame

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_vulkanLogicalDevice, &semaphoreInfo, g_allocator, &m_vulkanImageAvailableSemaphores[i]) != VK_SUCCESS 
            || vkCreateSemaphore(m_vulkanLogicalDevice, &semaphoreInfo, g_allocator, &m_vulkanRenderFinishedSemaphores[i]) != VK_SUCCESS 
            || vkCreateFence(m_vulkanLogicalDevice, &fenceInfo, g_allocator, &m_vulkanInFlightFences[i]) != VK_SUCCESS
        )
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
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
        DrawFrame();
    }

    //Wait until all vulkan operations are finished
    vkDeviceWaitIdle(m_vulkanLogicalDevice);
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::DrawFrame() {
    //The fence will sync CPU - GPU. Make sure that we are not processing the same frame in flight
    vkWaitForFences(m_vulkanLogicalDevice, 1, &m_vulkanInFlightFences[m_vulkanCurrentFrame], VK_TRUE, UINT64_MAX);

    //Acquire an image from the swap chain
    VkSemaphore curImageAvailableSemaphore = m_vulkanImageAvailableSemaphores[m_vulkanCurrentFrame];
    uint32_t imageIndex;
    {
        const VkResult result = vkAcquireNextImageKHR(m_vulkanLogicalDevice, m_vulkanSwapChain, UINT64_MAX, 
                                                      curImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateVulkanSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
    }

    //Check if we are about to draw to an image from the swap chain that is still in flight.
    //This can happen for example if MAX_FRAMES_IN_FLIGHT >= the number of images in the swap chain.
    if (m_vulkanImagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_vulkanLogicalDevice, 1, &m_vulkanImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    m_vulkanImagesInFlight[imageIndex] = m_vulkanInFlightFences[m_vulkanCurrentFrame];

    //Semaphores: GPU-GPU synchronization. No need to reset
    VkSemaphore waitSemaphores[] = {curImageAvailableSemaphore};
    VkSemaphore signalSemaphores[] = {m_vulkanRenderFinishedSemaphores[m_vulkanCurrentFrame]};

    //Execute the command buffer with that image as attachment in the framebuffer
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores; //Wait for the acquire process to finish
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_vulkanCommandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    //Reset fence for syncing sync CPU - GPU
    vkResetFences(m_vulkanLogicalDevice, 1, &m_vulkanInFlightFences[m_vulkanCurrentFrame]);

    if (vkQueueSubmit(m_vulkanGraphicsQueue, 1, &submitInfo, m_vulkanInFlightFences[m_vulkanCurrentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    //Return the image to the swap chain for presentation. Wait for rendering to be finished
    VkPresentInfoKHR presentInfo = {};
    VkSwapchainKHR swapChains[] = {m_vulkanSwapChain};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional
    {
        const VkResult result = vkQueuePresentKHR(m_vulkanPresentationQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_recreateSwapChainRequested) {
            //The recreateSwapChainRequested check is put here to make sure that the semaphores are in consistent state
            RecreateVulkanSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }

    m_vulkanCurrentFrame = (m_vulkanCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

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
VkSurfaceFormatKHR TriangleApp::PickVulkanSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* availableFormats) {

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

VkPresentModeKHR TriangleApp::PickVulkanSwapPresentMode(const std::vector<VkPresentModeKHR>* availableModes) {
    for (const VkPresentModeKHR& availablePresentMode : *availableModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR; //guaranteed to be available
}

//---------------------------------------------------------------------------------------------------------------------

VkExtent2D  TriangleApp::PickVulkanSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities) {

    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

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

void TriangleApp::CleanUp() {

    //Semaphores
    const uint32_t numSyncObjects = static_cast<uint32_t>(m_vulkanImageAvailableSemaphores.size());
    for (size_t i = 0; i < numSyncObjects; i++) {
        vkDestroySemaphore(m_vulkanLogicalDevice, m_vulkanImageAvailableSemaphores[i], g_allocator);
        vkDestroySemaphore(m_vulkanLogicalDevice, m_vulkanRenderFinishedSemaphores[i], g_allocator);
        vkDestroyFence(m_vulkanLogicalDevice, m_vulkanInFlightFences[i], g_allocator);
    }
    m_vulkanImageAvailableSemaphores.clear();
    m_vulkanRenderFinishedSemaphores.clear();
    m_vulkanInFlightFences.clear();

    CleanUpVulkanSwapChain();

    //Vertex Buffers
    if (nullptr != m_vulkanVB) {
        vkDestroyBuffer(m_vulkanLogicalDevice, m_vulkanVB, g_allocator);
        m_vulkanVB = nullptr;
    }
    if (nullptr != m_vulkanVBMemory) {
        vkFreeMemory(m_vulkanLogicalDevice, m_vulkanVBMemory, g_allocator);
        m_vulkanVBMemory = nullptr;
    }


    if (nullptr != m_vulkanCommandPool) {
        vkDestroyCommandPool(m_vulkanLogicalDevice, m_vulkanCommandPool, g_allocator);
        m_vulkanCommandPool = nullptr;
    }


    m_vulkanGraphicsQueue = nullptr;
    m_vulkanPresentationQueue = nullptr;



    if (nullptr != m_vulkanLogicalDevice) {
        vkDestroyDevice(m_vulkanLogicalDevice, g_allocator);
        m_vulkanLogicalDevice = nullptr;    
    }

    if (nullptr != m_vulkanInstance) {

        if (nullptr != m_vulkanSurface) {
            vkDestroySurfaceKHR(m_vulkanInstance, m_vulkanSurface, g_allocator);
            m_vulkanSurface = nullptr;
        }
#ifdef ENABLE_VULKAN_DEBUG
        m_vulkanDebug.Shutdown(m_vulkanInstance);
#endif
        vkDestroyInstance(m_vulkanInstance, g_allocator);
        m_vulkanInstance = nullptr;
    }

    if (nullptr != m_window) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = nullptr;
    }
   
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CleanUpVulkanSwapChain() {

    vkFreeCommandBuffers(m_vulkanLogicalDevice, m_vulkanCommandPool, static_cast<uint32_t>(m_vulkanCommandBuffers.size()), m_vulkanCommandBuffers.data());

    for (VkFramebuffer& framebuffer : m_vulkanSwapChainFramebuffers) {
        vkDestroyFramebuffer(m_vulkanLogicalDevice, framebuffer, g_allocator);
    }
    m_vulkanSwapChainFramebuffers.clear();

    for (VkImageView& imageView : m_vulkanSwapChainImageViews) {
        vkDestroyImageView(m_vulkanLogicalDevice, imageView, g_allocator);
    }
    m_vulkanSwapChainImages.clear();


    if (nullptr != m_vulkanGraphicsPipeline) {
        vkDestroyPipeline(m_vulkanLogicalDevice, m_vulkanGraphicsPipeline, g_allocator);
        m_vulkanGraphicsPipeline = nullptr;
    }

    if (nullptr != m_vulkanPipelineLayout) {
        vkDestroyPipelineLayout(m_vulkanLogicalDevice, m_vulkanPipelineLayout, g_allocator);
        m_vulkanPipelineLayout = nullptr;
    }

    if (nullptr != m_vulkanRenderPass) {
        vkDestroyRenderPass(m_vulkanLogicalDevice, m_vulkanRenderPass, g_allocator);
        m_vulkanRenderPass = nullptr;
    }
    if (nullptr!= m_vulkanSwapChain) {
        vkDestroySwapchainKHR(m_vulkanLogicalDevice, m_vulkanSwapChain, g_allocator);
    }
}
