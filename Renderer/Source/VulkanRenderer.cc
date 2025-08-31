
#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#include "VulkanRenderer.hh"

#ifdef VK_USE_PLATFORM_WIN32_KHR

#include <Windows.h>
#endif  

#include <iostream>
#include <limits>
#include <unordered_set>
#include <vector>

#include "ShaderManager.hh"

using namespace SilverBell::Renderer;

namespace
{
#if VULKAN_DEBUG_ENABLE
    const bool EnableValidationLayers = true;
#else
    const bool EnableValidationLayers = false;
#endif

    const std::vector<const char*> ValidationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> DeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo()
    {
        VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo = {};
        DebugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        DebugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        DebugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        //debugCreateInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)DebugCallback;

        DebugCreateInfo.pfnUserCallback =
            [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData) -> VkBool32
            {
                std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';

                return VK_FALSE;
            };

        return DebugCreateInfo;
    }

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& AvailableFormats)
    {
        if (AvailableFormats.size() == 1 && AvailableFormats[0].format == VK_FORMAT_UNDEFINED)
        {
            return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        }

        for (const auto& Format : AvailableFormats)
        {
            if (Format.format == VK_FORMAT_B8G8R8A8_UNORM && Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return Format;
            }
        }
        return AvailableFormats[0]; // 返回第一个可用格式
    }

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& AvailablePresentModes)
    {
        VkPresentModeKHR BestMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // 默认使用 FIFO 模式

        for (const auto& Mode : AvailablePresentModes)
        {
            if (Mode == VK_PRESENT_MODE_FIFO_KHR)
            {
                return Mode;
            }
        }
        return BestMode; // 如果没有 Mailbox 模式，使用 FIFO 模式
    }

    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& Capabilities, int Width, int Height)
    {
        if (Capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return Capabilities.currentExtent; // 返回当前的交换链范围
        }
        else
        {
            VkExtent2D ActualExtent = { static_cast<uint32_t>(Width), static_cast<uint32_t>(Height) };
            ActualExtent.width = std::max(Capabilities.minImageExtent.width, std::min(Capabilities.maxImageExtent.width, ActualExtent.width));
            ActualExtent.height = std::max(Capabilities.minImageExtent.height, std::min(Capabilities.maxImageExtent.height, ActualExtent.height));
            return ActualExtent;
        }
    }

}

FVulkanRenderer::FVulkanRenderer() :
    Instance(VK_NULL_HANDLE),
    DebugMessenger(VK_NULL_HANDLE),
    PhysicalDevice(VK_NULL_HANDLE),
    LogicalDevice(VK_NULL_HANDLE),
    GraphicsQueue(VK_NULL_HANDLE),
    PresentQueue(VK_NULL_HANDLE),
    Surface(VK_NULL_HANDLE),
    SwapChain(VK_NULL_HANDLE),
    SwapChainImageFormat(VK_FORMAT_UNDEFINED),
    SwapChainExtent({ 0, 0 })
{
}

FVulkanRenderer::~FVulkanRenderer()
{
    CleanUp();
}

void FVulkanRenderer::Initialize()
{
    CreateInstance();
    if constexpr (EnableValidationLayers)
    {
        SetupDebugMessenger();
    }
    PickPhysicalDevice();
    CreateLogicalDevice();
}

void FVulkanRenderer::SetRequiredInstanceExtensions(const char** Exts, int Len)
{
    InstanceExtensions.reserve(Len);
    for (int Idx = 0; Idx < Len; ++Idx)
    {
        InstanceExtensions.push_back(Exts[Idx]);
    }
}

void FVulkanRenderer::CleanUp()
{
    if (DebugMessenger != VK_NULL_HANDLE)
    {
        vkDestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
        DebugMessenger = VK_NULL_HANDLE;
    }

    vkDestroySwapchainKHR(LogicalDevice, SwapChain, nullptr);
    for (auto ImageView : SwapChainImageViews)
    {
        vkDestroyImageView(LogicalDevice, ImageView, nullptr);
    }
    vkDestroyPipeline(LogicalDevice, GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);
    vkDestroyCommandPool(LogicalDevice, CommandPool, nullptr);
    vkDestroyDevice(LogicalDevice, nullptr);
    vkDestroySurfaceKHR(Instance, Surface, nullptr);

    if (Instance != nullptr)
    {
        vkDestroyInstance(Instance, nullptr);
        Instance = nullptr;
    }

}

void FVulkanRenderer::DrawFrame()
{

    vkQueueWaitIdle(PresentQueue);

    uint32_t ImageIndex;
    auto Result = vkAcquireNextImageKHR(LogicalDevice, SwapChain, std::numeric_limits<uint64_t>::max(), ImageAvailableSemaphore, VK_NULL_HANDLE, &ImageIndex);
    // TODO ： 这里会有失败的情况，需要寻找原因
    if (Result != VK_SUCCESS)
        return;

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore WaitSemaphores[] = { ImageAvailableSemaphore };
    VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    SubmitInfo.waitSemaphoreCount = 1;
    SubmitInfo.pWaitSemaphores = WaitSemaphores;
    SubmitInfo.pWaitDstStageMask = WaitStages;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CommandBuffers[ImageIndex];
    VkSemaphore SignalSemaphores[] = { RenderFinishedSemaphore };
    SubmitInfo.signalSemaphoreCount = 1;
    SubmitInfo.pSignalSemaphores = SignalSemaphores;
    if (vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    // 提交呈现请求
    VkPresentInfoKHR PresentInfo = {};
    PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.waitSemaphoreCount = 1;
    PresentInfo.pWaitSemaphores = SignalSemaphores;
    VkSwapchainKHR SwapChains[] = { SwapChain };
    PresentInfo.swapchainCount = 1;
    PresentInfo.pSwapchains = SwapChains;
    PresentInfo.pImageIndices = &ImageIndex;
    PresentInfo.pResults = nullptr; // Optional
    vkQueuePresentKHR(PresentQueue, &PresentInfo);

}

void FVulkanRenderer::CreateInstance()
{
    if (volkInitialize() != VK_SUCCESS)
    {
        return;
    }

    if constexpr (EnableValidationLayers && !CheckValidationLayerSupport())
    {
        return;
    }

    VkApplicationInfo AppInfo = {};
    AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pNext = nullptr;
    AppInfo.pApplicationName = "Silver Bell Engine";
    AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    AppInfo.pEngineName = "Silver Bell Engine";
    AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    AppInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo InstanceCreateInfo = {};
    InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceCreateInfo.pNext = nullptr;
    InstanceCreateInfo.flags = 0;
    InstanceCreateInfo.pApplicationInfo = &AppInfo;

    if (EnableValidationLayers)
    {
        InstanceCreateInfo.enabledLayerCount = ValidationLayers.size();
        InstanceCreateInfo.ppEnabledLayerNames = ValidationLayers.data();
        auto DebugCreateInfo = GetDebugMessengerCreateInfo();
        InstanceCreateInfo.pNext = &DebugCreateInfo;
    }
    else
    {
        InstanceCreateInfo.enabledLayerCount = 0;
        InstanceCreateInfo.ppEnabledLayerNames = nullptr;
    }

    InstanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(InstanceExtensions.size());
    InstanceCreateInfo.ppEnabledExtensionNames = InstanceExtensions.data();

    VkResult Result = vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance);
    if (Result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan instance: " << Result << '\n';
        std::exit(0);
    }

    volkLoadInstance(Instance);

}

void FVulkanRenderer::CreateSurface(void* WindowHandle)
{

#ifdef VK_USE_PLATFORM_WIN32_KHR

    VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {};
    SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
    SurfaceCreateInfo.hwnd = reinterpret_cast<HWND>(WindowHandle);
    if (vkCreateWin32SurfaceKHR(Instance, &SurfaceCreateInfo, nullptr, &Surface) != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan surface!" << '\n';
    }

#endif

}

void FVulkanRenderer::PickPhysicalDevice()
{
    uint32_t DeviceCount = 0;
    vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr);
    if (DeviceCount == 0)
    {
        std::cerr << "No Vulkan-compatible devices found!" << '\n';
        return;
    }
    std::vector<VkPhysicalDevice> Devices(DeviceCount);
    vkEnumeratePhysicalDevices(Instance, &DeviceCount, Devices.data());

    // TODO : 如果有多个设备可以评估选择最优

    for (const auto& Device : Devices)
    {
        if (IsDeviceSuitable(Device))
        {
            PhysicalDevice = Device;
            break;
        }
    }

    if (PhysicalDevice == VK_NULL_HANDLE)
    {
        std::cerr << "Failed to find a suitable GPU!" << '\n';
    }
}

bool FVulkanRenderer::IsDeviceSuitable(VkPhysicalDevice Device)
{
    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(Device, &DeviceProperties);
    VkPhysicalDeviceFeatures DeviceFeatures;
    vkGetPhysicalDeviceFeatures(Device, &DeviceFeatures);

    // 检查队列簇
    const auto FamilyIndices = FindQueueFamilies(Device);

    // 检查物理设备的扩展支持
    bool bExtensionsSupported = CheckDeviceExtensionSupport(Device);

    // 检查交换链支持
    bool bSwapChainAdequate = false;
    if (bExtensionsSupported)
    {
        SwapChainSupportDetails SwapChainDetails = QuerySwapChainSupport(Device);
        bSwapChainAdequate = !SwapChainDetails.Formats.empty() && !SwapChainDetails.PresentModes.empty();
    }

    // 检查设备是否支持必要的功能
    return (DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) &&
        DeviceFeatures.geometryShader && FamilyIndices.IsComplete() &&
        bExtensionsSupported && bSwapChainAdequate; // 这里可以添加更多的检查条件
}

bool FVulkanRenderer::CheckValidationLayerSupport()
{
    uint32_t LayerCount;
    vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
    if (LayerCount == 0)
    {
        std::cerr << "No validation layers supported!" << '\n';
        return false;
    }
    std::vector<VkLayerProperties> AvailableLayers(LayerCount);
    vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());
    for (const auto& Layer : AvailableLayers)
    {
        if (strcmp(Layer.layerName, "VK_LAYER_KHRONOS_validation") == 0)
        {
            return true;
        }
    }
    return false;
}

bool FVulkanRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice Device)
{
    std::uint32_t ExtensionCount;
    vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, nullptr);
    std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
    vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, AvailableExtensions.data());

    // 这里不能用const char*来比较
    std::unordered_set<std::string> RequiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());
    for (const auto& Extension : AvailableExtensions)
    {
        RequiredExtensions.erase(Extension.extensionName);
    }

    return RequiredExtensions.empty();
}

void FVulkanRenderer::SetupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo = GetDebugMessengerCreateInfo();
    if (vkCreateDebugUtilsMessengerEXT(Instance, &DebugCreateInfo, nullptr, &DebugMessenger) != VK_SUCCESS)
    {

    }
}

void FVulkanRenderer::CreateLogicalDevice()
{
    // 指定创建的队列
    QueueFamilyIndices FamilyIndices = FindQueueFamilies(PhysicalDevice);
    if (!FamilyIndices.IsComplete())
    {
        throw std::runtime_error("Queue family indices are not complete!");
    }
    float QueuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
    std::unordered_set UniqueQueueFamilies = { FamilyIndices.GraphicsFamily.value(), FamilyIndices.PresentFamily.value() };
    for (uint32_t FamilyIndex : UniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo QueueCreateInfo = {};
        QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfo.queueFamilyIndex = FamilyIndex;
        QueueCreateInfo.queueCount = 1; // 每个队列族只创建一个队列
        QueueCreateInfo.pQueuePriorities = &QueuePriority; // 队列优先级
        QueueCreateInfos.push_back(QueueCreateInfo);
    }

    // 指定使用的设备特性
    VkPhysicalDeviceFeatures DeviceFeatures = {};
    DeviceFeatures.geometryShader = VK_TRUE; // 启用几何着色器
    DeviceFeatures.fillModeNonSolid = VK_TRUE; // 启用非实心填充模式

    // 创建逻辑设备
    VkDeviceCreateInfo DeviceCreateInfo = {};
    DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());
    DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
    DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

    DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
    DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();
    if (EnableValidationLayers)
    {
        
    }
    else
    {
        DeviceCreateInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &LogicalDevice) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device!");
    }
    if (QueueCreateInfos.size() == 1)
    {
        vkGetDeviceQueue(LogicalDevice, FamilyIndices.PresentFamily.value(), 0, &PresentQueue);
        GraphicsQueue = PresentQueue; // 如果图形队列和呈现队列是同一个
    }
    else
    {
        vkGetDeviceQueue(LogicalDevice, FamilyIndices.PresentFamily.value(), 0, &PresentQueue);
        vkGetDeviceQueue(LogicalDevice, FamilyIndices.GraphicsFamily.value(), 0, &GraphicsQueue);
    }

    // 创建VMA内存分配器
    CreateMemoryAllocator();
   
}

void FVulkanRenderer::CreateSwapChain(int Width, int Height)
{
    SwapChainSupportDetails SwapChainDetails = QuerySwapChainSupport(PhysicalDevice);
    if (SwapChainDetails.Formats.empty() || SwapChainDetails.PresentModes.empty())
    {
        throw std::runtime_error("Swap chain details are not available!");
    }
    VkSurfaceFormatKHR SurfaceFormat = ChooseSwapSurfaceFormat(SwapChainDetails.Formats);
    VkPresentModeKHR PresentMode = ChooseSwapPresentMode(SwapChainDetails.PresentModes);
    VkExtent2D Extent = ChooseSwapExtent(SwapChainDetails.Capabilities, Width, Height);
    uint32_t ImageCount = SwapChainDetails.Capabilities.minImageCount + 1;
    if (SwapChainDetails.Capabilities.maxImageCount > 0 && ImageCount > SwapChainDetails.Capabilities.maxImageCount)
    {
        ImageCount = SwapChainDetails.Capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR SwapChainCreateInfo = {};
    SwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapChainCreateInfo.surface = Surface; // 假设已经创建了VkSurfaceKHR
    SwapChainCreateInfo.minImageCount = ImageCount;
    SwapChainCreateInfo.imageFormat = SurfaceFormat.format;
    SwapChainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
    SwapChainCreateInfo.imageExtent = Extent;
    SwapChainCreateInfo.imageArrayLayers = 1;
    SwapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    QueueFamilyIndices FamilyIndices = FindQueueFamilies(PhysicalDevice);
    uint32_t QueueFamilyIndicesArray[] = { FamilyIndices.GraphicsFamily.value(), FamilyIndices.PresentFamily.value() };
    
    if (FamilyIndices.GraphicsFamily != FamilyIndices.PresentFamily)
    {
        SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        SwapChainCreateInfo.queueFamilyIndexCount = 2;
        SwapChainCreateInfo.pQueueFamilyIndices = QueueFamilyIndicesArray;
    }
    else
    {
        SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        SwapChainCreateInfo.queueFamilyIndexCount = 0; // 不使用共享队列
        SwapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }
    SwapChainCreateInfo.preTransform = SwapChainDetails.Capabilities.currentTransform;
    SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // 假设使用不透明合成

    SwapChainCreateInfo.presentMode = PresentMode;
    SwapChainCreateInfo.clipped = VK_TRUE; // 允许裁剪
    SwapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // 如果是第一次创建交换链，设置为VK_NULL_HANDLE
    if (vkCreateSwapchainKHR(LogicalDevice, &SwapChainCreateInfo, nullptr, &SwapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain!");
    }

    // 获取交换链图像
    vkGetSwapchainImagesKHR(LogicalDevice, SwapChain, &ImageCount, nullptr);
    SwapChainImages.resize(ImageCount);
    vkGetSwapchainImagesKHR(LogicalDevice, SwapChain, &ImageCount, SwapChainImages.data());
    SwapChainImageFormat = SurfaceFormat.format;
    SwapChainExtent = Extent;
}

void FVulkanRenderer::CreateImageViews()
{
    SwapChainImageViews.resize(SwapChainImages.size());
    for (size_t i = 0; i < SwapChainImages.size(); ++i)
    {
        VkImageViewCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        CreateInfo.image = SwapChainImages[i];
        CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        CreateInfo.format = SwapChainImageFormat;
        CreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        CreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        CreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        CreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        CreateInfo.subresourceRange.baseMipLevel = 0;
        CreateInfo.subresourceRange.levelCount = 1;
        CreateInfo.subresourceRange.baseArrayLayer = 0;
        CreateInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(LogicalDevice, &CreateInfo, nullptr, &SwapChainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

void FVulkanRenderer::CreateRenderPass()
{
    VkAttachmentDescription ColorAttachment = {};
    ColorAttachment.format = SwapChainImageFormat;
    ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 使用1倍采样
    ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // 清除颜色附件
    ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // 存储颜色附件
    ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // 忽略模板附件加载
    ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 忽略模板附件存储
    ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // 初始布局为未定义
    ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // 最终布局为呈现源
    VkAttachmentReference ColorAttachmentRef = {};
    ColorAttachmentRef.attachment = 0; // 附件索引
    ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 颜色附件布局
    VkSubpassDescription Subpass = {};
    Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // 图形管线绑定点
    Subpass.colorAttachmentCount = 1;
    Subpass.pColorAttachments = &ColorAttachmentRef;
    VkSubpassDependency Dependency = {};
    Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    Dependency.dstSubpass = 0;
    Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    Dependency.srcAccessMask = 0;
    Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = 1;
    RenderPassInfo.pAttachments = &ColorAttachment;
    RenderPassInfo.subpassCount = 1;
    RenderPassInfo.pSubpasses = &Subpass;
    RenderPassInfo.dependencyCount = 1;
    RenderPassInfo.pDependencies = &Dependency;
    if (vkCreateRenderPass(LogicalDevice, &RenderPassInfo, nullptr, &RenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void FVulkanRenderer::CreateGraphicsPipeline()
{
    // 创建着色器模块
    auto FragmentShaderModule = FShaderManager::Instance().CreateShaderModule("Assets/Shaders/Triangle/SPIR-V/TrianglePS.spv", LogicalDevice);
    auto VertexShaderModule = FShaderManager::Instance().CreateShaderModule("Assets/Shaders/Triangle/SPIR-V/TriangleVS.spv", LogicalDevice);
    VkPipelineShaderStageCreateInfo VertexShaderStageInfo = {};
    VertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    VertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    VertexShaderStageInfo.module = VertexShaderModule;
    VertexShaderStageInfo.pName = "Main"; // 入口点名称
    VkPipelineShaderStageCreateInfo FragmentShaderStageInfo = {};
    FragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    FragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    FragmentShaderStageInfo.module = FragmentShaderModule;
    FragmentShaderStageInfo.pName = "Main"; // 入口点名称
    VkPipelineShaderStageCreateInfo ShaderStages[] = { VertexShaderStageInfo, FragmentShaderStageInfo };

    // 顶点输入状态
    VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
    VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputInfo.vertexBindingDescriptionCount = 0; // 如果没有顶点绑定描述符
    VertexInputInfo.pVertexBindingDescriptions = nullptr; // 没有顶点绑定描述符
    VertexInputInfo.vertexAttributeDescriptionCount = 0; // 如果没有顶点属性描述符
    VertexInputInfo.pVertexAttributeDescriptions = nullptr; // 没有顶点属性描述符
    // 输入装配状态
    VkPipelineInputAssemblyStateCreateInfo InputAssembly = {};
    InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // 假设使用三角形列表
    InputAssembly.primitiveRestartEnable = VK_FALSE; // 不启用原始重启
    // 视口和裁剪
    VkViewport Viewport = {};
    Viewport.x = 0.0f;
    Viewport.y = 0.0f;
    Viewport.width = static_cast<float>(SwapChainExtent.width);
    Viewport.height = static_cast<float>(SwapChainExtent.height);
    Viewport.minDepth = 0.0f;
    Viewport.maxDepth = 1.0f;
    VkRect2D Scissor = {};
    Scissor.offset = { 0, 0 };
    Scissor.extent = SwapChainExtent;
    VkPipelineViewportStateCreateInfo ViewportState = {};
    ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportState.viewportCount = 1;
    ViewportState.pViewports = &Viewport;
    ViewportState.scissorCount = 1;
    ViewportState.pScissors = &Scissor;
    // 光栅化状态
    VkPipelineRasterizationStateCreateInfo Rasterizer = {};
    Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    Rasterizer.depthClampEnable = VK_FALSE; // 不启用深度夹紧
    Rasterizer.rasterizerDiscardEnable = VK_FALSE; // 不丢弃片段
    Rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // 填充模式
    Rasterizer.lineWidth = 1.0f; // 线宽
    Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // 背面剔除
    Rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // 顺时针为前面
    Rasterizer.depthBiasEnable = VK_FALSE; // 不启用深度偏移
    Rasterizer.depthBiasConstantFactor = 0.0f; // 深度偏移常数因子
    Rasterizer.depthBiasClamp = 0.0f; // 深度偏移夹紧
    Rasterizer.depthBiasSlopeFactor = 0.0f; // 深度偏移斜率因子
    // 多重采样状态
    VkPipelineMultisampleStateCreateInfo Multisampling = {};
    Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    Multisampling.sampleShadingEnable = VK_FALSE; // 不启用采样着色
    Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // 使用1倍采样
    Multisampling.minSampleShading = 1.0f; // 最小采样着色比率
    Multisampling.pSampleMask = nullptr; // 不使用采样掩码
    Multisampling.alphaToCoverageEnable = VK_FALSE; // 不启用alpha到覆盖
    Multisampling.alphaToOneEnable = VK_FALSE; // 不启用alpha到1
    // 深度和模板测试状态

    // 颜色和混合状态
    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.blendEnable = VK_FALSE; // 不启用混合
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; // 写入所有颜色分量
    ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // 源颜色混合因子
    ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // 目标颜色混合因子
    ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // 颜色混合操作
    ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // 源alpha混合因子
    ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // 目标alpha混合因子
    ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // alpha混合操作
    // 全局混合状态
    VkPipelineColorBlendStateCreateInfo ColorBlending = {};
    ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlending.logicOpEnable = VK_FALSE; // 不启用逻辑操作
    ColorBlending.logicOp = VK_LOGIC_OP_COPY; // 逻辑操作
    ColorBlending.attachmentCount = 1;
    ColorBlending.pAttachments = &ColorBlendAttachment;
    ColorBlending.blendConstants[0] = 0.0f; // 混合常数
    ColorBlending.blendConstants[1] = 0.0f;
    ColorBlending.blendConstants[2] = 0.0f;
    ColorBlending.blendConstants[3] = 0.0f;
    // 动态修改
  /*  VkDynamicState DynamicStates[] = 
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };*/
    VkPipelineDynamicStateCreateInfo DynamicState = {};
    DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicState.dynamicStateCount = 0; // 视口和裁剪
    //DynamicState.pDynamicStates = DynamicStates;
    // 创建管线布局
    VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
    PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutInfo.setLayoutCount = 0; // 如果没有描述符集布局
    PipelineLayoutInfo.pSetLayouts = nullptr; // 没有描述符集布局
    PipelineLayoutInfo.pushConstantRangeCount = 0; // 如果没有推送常量范围
    PipelineLayoutInfo.pPushConstantRanges = nullptr; // 没有推送常量范围
    if (vkCreatePipelineLayout(LogicalDevice, &PipelineLayoutInfo, nullptr, &PipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo PipelineCreateInfo = {};
    PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // 假设已经创建了顶点和片段着色器模块
    PipelineCreateInfo.stageCount = 2;
    PipelineCreateInfo.pStages = ShaderStages;
    PipelineCreateInfo.pVertexInputState = &VertexInputInfo;
    PipelineCreateInfo.pInputAssemblyState = &InputAssembly;
    PipelineCreateInfo.pViewportState = &ViewportState;
    PipelineCreateInfo.pRasterizationState = &Rasterizer;
    PipelineCreateInfo.pMultisampleState = &Multisampling;
    PipelineCreateInfo.pDepthStencilState = nullptr; // 如果没有深度和模板测试
    PipelineCreateInfo.pColorBlendState = &ColorBlending;
    PipelineCreateInfo.pDynamicState = &DynamicState; // 如果没有动态状态
    PipelineCreateInfo.layout = PipelineLayout; // 假设已经创建了管线布局
    PipelineCreateInfo.renderPass = RenderPass; // 假设已经创建了渲
    PipelineCreateInfo.subpass = 0; // 使用第一个子通道
    PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // 不使用基础管线
    PipelineCreateInfo.basePipelineIndex = -1; // 不使用基础管线索引

    if (vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, 1, &PipelineCreateInfo,
        nullptr, &GraphicsPipeline)!= VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
}

void FVulkanRenderer::CreateCommandPool()
{
    QueueFamilyIndices FamilyIndices = FindQueueFamilies(PhysicalDevice);
    if (!FamilyIndices.IsComplete())
    {
        throw std::runtime_error("Queue family indices are not complete!");
    }
    VkCommandPoolCreateInfo PoolCreateInfo = {};
    PoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    PoolCreateInfo.flags = 0;
    PoolCreateInfo.queueFamilyIndex = FamilyIndices.GraphicsFamily.value(); // 使用图形队列族
    if (vkCreateCommandPool(LogicalDevice, &PoolCreateInfo, nullptr, &CommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void FVulkanRenderer::CreateCommandBuffers()
{
    CommandBuffers.resize(SwapChainFramebuffers.size());
    VkCommandBufferAllocateInfo AllocateInfo = {};
    AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocateInfo.commandPool = CommandPool;
    AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // 主命令缓冲
    AllocateInfo.commandBufferCount = static_cast<uint32_t>(CommandBuffers.size());
    if (vkAllocateCommandBuffers(LogicalDevice, &AllocateInfo, CommandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
    for (size_t i = 0; i < CommandBuffers.size(); ++i)
    {
        VkCommandBufferBeginInfo BeginInfo = {};
        BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // 可选标志
        BeginInfo.pInheritanceInfo = nullptr; // 可选继承信息
        if (vkBeginCommandBuffer(CommandBuffers[i], &BeginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }
        
        VkRenderPassBeginInfo RenderPassInfo = {};
        RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassInfo.renderPass = RenderPass;
        RenderPassInfo.framebuffer = SwapChainFramebuffers[i];
        RenderPassInfo.renderArea.offset = { 0, 0 };
        RenderPassInfo.renderArea.extent = SwapChainExtent;
        VkClearValue ClearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} }; // 清除颜色为黑色
        RenderPassInfo.clearValueCount = 1;
        RenderPassInfo.pClearValues = &ClearColor;
        vkCmdBindPipeline(CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);
        vkCmdBeginRenderPass(CommandBuffers[i], &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDraw(CommandBuffers[i], 3, 1, 0, 0); // 绘制一个三角形
        vkCmdEndRenderPass(CommandBuffers[i]);
        if (vkEndCommandBuffer(CommandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }
}

void FVulkanRenderer::CreateSemaphores()
{
    VkSemaphoreCreateInfo SemaphoreInfo = {};
    SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(LogicalDevice, &SemaphoreInfo, nullptr, &ImageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(LogicalDevice, &SemaphoreInfo, nullptr, &RenderFinishedSemaphore) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create semaphores!");
    }
}

void FVulkanRenderer::CreateFramebuffers()
{
    SwapChainFramebuffers.resize(SwapChainImageViews.size());

    for (size_t i = 0; i < SwapChainImageViews.size(); ++i)
    {
        VkFramebufferCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        CreateInfo.renderPass = VK_NULL_HANDLE; // 需要先创建渲染通道
        CreateInfo.attachmentCount = 1;
        CreateInfo.pAttachments = &SwapChainImageViews[i];
        CreateInfo.width = SwapChainExtent.width;
        CreateInfo.height = SwapChainExtent.height;
        CreateInfo.layers = 1;
        if (vkCreateFramebuffer(LogicalDevice, &CreateInfo, nullptr, &SwapChainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

FVulkanRenderer::SwapChainSupportDetails FVulkanRenderer::QuerySwapChainSupport(VkPhysicalDevice Device)
{

    SwapChainSupportDetails Details;

    // 查询表面能力
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device, Surface, &Details.Capabilities);
    // 查询表面格式
    uint32_t FormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatCount, nullptr);
    if (FormatCount != 0)
    {
        Details.Formats.resize(FormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatCount, Details.Formats.data());
    }
    // 查询表面呈现模式
    uint32_t PresentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &PresentModeCount, nullptr);
    if (PresentModeCount != 0)
    {
        Details.PresentModes.resize(PresentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &PresentModeCount, Details.PresentModes.data());
    }

    return Details;
}

FVulkanRenderer::QueueFamilyIndices FVulkanRenderer::FindQueueFamilies(VkPhysicalDevice Device)
{
    QueueFamilyIndices Indices;

    uint32_t QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, QueueFamilies.data());

    int Idx = 0;
    for (const auto& QueueFamily : QueueFamilies)
    {
        // 检查队列族是否支持图形和呈现操作
        VkBool32 PresentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(Device, Idx, Surface, &PresentSupport);

        if (QueueFamily.queueCount > 0)
        {
            if (PresentSupport)
            {
                Indices.PresentFamily = Idx;
            }
            if (QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                Indices.GraphicsFamily = Idx;
            }
        }
        if (Indices.IsComplete())
        {
            break; // 如果找到了所有需要的队列族，提前退出循环
        }
        ++Idx;
    }

    return Indices;
}

void FVulkanRenderer::CreateMemoryAllocator()
{
    // volk集成vma: https://zhuanlan.zhihu.com/p/634912614 
    // vma官方文档: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/index.html
    // 对照该表，不同vulkan版本有不同的绑定要求: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/struct_vma_vulkan_functions.html

    VmaVulkanFunctions VulkanFunctions{};
    VulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    VulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    VulkanFunctions.vkAllocateMemory = vkAllocateMemory;
    VulkanFunctions.vkFreeMemory = vkFreeMemory;
    VulkanFunctions.vkMapMemory = vkMapMemory;
    VulkanFunctions.vkUnmapMemory = vkUnmapMemory;
    VulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    VulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    VulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
    VulkanFunctions.vkBindImageMemory = vkBindImageMemory;
    VulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    VulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    VulkanFunctions.vkCreateBuffer = vkCreateBuffer;
    VulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
    VulkanFunctions.vkCreateImage = vkCreateImage;
    VulkanFunctions.vkDestroyImage = vkDestroyImage;
    VulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
    // vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
    // vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;
    // vulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
    // vulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
    // vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
    VulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
    VulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
    VulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
    VulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2;
    VulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
    VulkanFunctions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
    VulkanFunctions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

    VmaAllocatorCreateInfo AllocatorCreateInfo = {};
    AllocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
    AllocatorCreateInfo.physicalDevice = PhysicalDevice;
    AllocatorCreateInfo.device = LogicalDevice;
    AllocatorCreateInfo.instance = Instance;
    AllocatorCreateInfo.pVulkanFunctions = &VulkanFunctions;
    AllocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // 加上这个才能获取地址

    vmaCreateAllocator(&AllocatorCreateInfo, &MemoryAllocator);
}
