
#define VOLK_IMPLEMENTATION

#define VMA_VULKAN_VERSION 1001000
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#include "VulkanRenderer.hh"

#define USE_VULKAN_VERSION VK_API_VERSION_1_1

#ifdef VK_USE_PLATFORM_WIN32_KHR

#include <Windows.h>
#endif  

#include <iostream>
#include <limits>
#include <unordered_set>
#include <vector>

#include "ImageImporter.hh"
#include "ShaderManager.hh"

#include <spdlog/spdlog.h>

#include "ModelImporter.hh"

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
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
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
        VkPresentModeKHR BestMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

        for (const auto& Mode : AvailablePresentModes)
        {
            // 默认使用 FIFO 模式
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
    SwapChainExtent({.width = 0, .height = 0 }),
    RenderPass(VK_NULL_HANDLE),
    GraphicsPipeline(VK_NULL_HANDLE),
    PipelineLayout(VK_NULL_HANDLE),
    CommandPool(VK_NULL_HANDLE),
    ImageAvailableSemaphore(VK_NULL_HANDLE),
    RenderFinishedSemaphore(VK_NULL_HANDLE),
    MemoryAllocator(VMA_NULL)
{
}

FVulkanRenderer::~FVulkanRenderer()
{
    CleanUp();

    if (LoadedModel)
    {
        delete LoadedModel;
        LoadedModel = nullptr;
    }
}

void FVulkanRenderer::SetRequiredInstanceExtensions(const char** Exts, int Len)
{
    InstanceExtensions.reserve(Len);
    for (int Idx = 0; Idx < Len; ++Idx)
    {
        InstanceExtensions.push_back(Exts[Idx]);
    }

    //InstanceExtensions.push_back("VK_KHR_buffer_device_address");
}

void FVulkanRenderer::CleanUp()
{
    vkDeviceWaitIdle(LogicalDevice);
    if (DebugMessenger != VK_NULL_HANDLE)
    {
        vkDestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
        DebugMessenger = VK_NULL_HANDLE;
    }
    CleanupSwapChain();
    // 销毁着色器模块
    for (auto ShaderModule : ShaderModules)
    {
        vkDestroyShaderModule(LogicalDevice, ShaderModule, nullptr);
    }

    // 销毁描述符集布局
    vkDestroyDescriptorSetLayout(LogicalDevice, DescriptorSetLayout, nullptr);
    // 销毁描述符池
    vkDestroyDescriptorPool(LogicalDevice, DescriptorPool, nullptr);

    vkDestroySemaphore(LogicalDevice, RenderFinishedSemaphore, nullptr);
    vkDestroySemaphore(LogicalDevice, ImageAvailableSemaphore, nullptr);
    vkDestroyCommandPool(LogicalDevice, CommandPool, nullptr);

    // 销毁顶点缓冲区
    for (auto& BufferCache : VertexBufferCaches)
    {
        vmaDestroyBuffer(MemoryAllocator, BufferCache.BufferHandle, BufferCache.Allocation);
    }
    // 销毁索引缓冲区
    for (auto& BufferCache : IndexBufferCaches)
    {
        vmaDestroyBuffer(MemoryAllocator, BufferCache.BufferHandle, BufferCache.Allocation);
    }
    // 销毁常量缓冲区
    for (auto& BufferCache : ConstantBufferCaches)
    {
        vmaDestroyBuffer(MemoryAllocator, BufferCache.BufferHandle, BufferCache.Allocation);
    }
    // 销毁纹理
    vmaDestroyImage(MemoryAllocator, TextureImageCache.ImageHandle, TextureImageCache.Allocation);
    vkDestroySampler(LogicalDevice, TextureSampler, nullptr);
    vkDestroyImageView(LogicalDevice, TextureImageView, nullptr);

    vmaDestroyAllocator(MemoryAllocator);

    vkDestroyDevice(LogicalDevice, nullptr);
    vkDestroySurfaceKHR(Instance, Surface, nullptr);
    if (Instance != nullptr)
    {
        vkDestroyInstance(Instance, nullptr);
        Instance = nullptr;
    }
}

bool FVulkanRenderer::DrawFrame()
{
    vkQueueWaitIdle(PresentQueue);

    std::uint32_t ImageIndex;
    auto Result = vkAcquireNextImageKHR(LogicalDevice, SwapChain, std::numeric_limits<uint64_t>::max(), ImageAvailableSemaphore, VK_NULL_HANDLE, &ImageIndex);

    // TODO ： 未实现Resize处理，窗口大小变化时会报错
    static VkResult LastError;
    if (Result != VK_SUCCESS)
    {
        if (Result != LastError)
        {
            LastError = Result;
            switch (LastError)  // NOLINT(clang-diagnostic-switch-enum)
            {
            case VK_ERROR_OUT_OF_DATE_KHR:
                spdlog::warn("交换链过时，需要重新创建！已停止渲染！");
                break;
            case VK_SUBOPTIMAL_KHR:
                spdlog::warn("交换链仍然可以向surface提交图像，但是surface的属性不再匹配准确！");
                break;
            default:
                spdlog::error("交换链发生未知错误！");
            }
        }
        return false;
    }

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
        spdlog::error("提交绘制命令缓冲区失败！");
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

    return true;
}

void FVulkanRenderer::CreateInstance()
{
    if (volkInitialize() != VK_SUCCESS)
    {
        spdlog::error("Volk初始化失败！");
        return;
    }

    if constexpr (EnableValidationLayers)
    {
        if (!CheckValidationLayerSupport())
        {
            // 失败日志
            spdlog::error("检查校验层支持失败！");
            return;
        }
    }

    VkApplicationInfo AppInfo = {};
    AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pNext = nullptr;
    AppInfo.pApplicationName = "Silver Bell Engine";
    AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    AppInfo.pEngineName = "Silver Bell Engine";
    AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    AppInfo.apiVersion = USE_VULKAN_VERSION;

    VkInstanceCreateInfo InstanceCreateInfo = {};
    InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceCreateInfo.pNext = nullptr;
    InstanceCreateInfo.flags = 0;
    InstanceCreateInfo.pApplicationInfo = &AppInfo;

    if (EnableValidationLayers)
    {
        InstanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
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
        spdlog::error("创建Vulkan实例失败，程序退出！");
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
        spdlog::error("创建Vulkan Surface失败！");
    }

#endif

}

void FVulkanRenderer::PickPhysicalDevice()
{
    uint32_t DeviceCount = 0;
    vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr);
    if (DeviceCount == 0)
    {
        spdlog::error("未找到兼容Vulkan的设备！");
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
        spdlog::error("未找到合适的GPU！");
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
        bExtensionsSupported && bSwapChainAdequate && DeviceFeatures.samplerAnisotropy; // 这里可以添加更多的检查条件
}

bool FVulkanRenderer::CheckValidationLayerSupport()
{
    uint32_t LayerCount;
    vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
    if (LayerCount == 0)
    {
        spdlog::error("不支持任何验证层！");
        return false;
    }
    std::vector<VkLayerProperties> AvailableLayers(LayerCount);
    vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());

    // 这里不能用const char*来比较
    std::unordered_set<std::string> RequiredLayers(ValidationLayers.begin(), ValidationLayers.end());
    for (const auto& Layer : AvailableLayers)
    {
        RequiredLayers.erase(Layer.layerName);
    }

    if (RequiredLayers.empty())
    {
        return true;
    }
    else
    {
        // 输出缺失的验证层
        for (const auto& LayerName : RequiredLayers)
        {
            spdlog::error("缺失验证层: {}", LayerName);
        }
        return false;
    }
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

    if (RequiredExtensions.empty())
    {
        return true;
    }
    else
    {
        // 输出缺失的设备扩展
        for (const auto& ExtensionName : RequiredExtensions)
        {
            spdlog::error("缺失设备扩展: {}", ExtensionName);
        }
        return false;
    }
}

void FVulkanRenderer::SetupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo = GetDebugMessengerCreateInfo();
    if (vkCreateDebugUtilsMessengerEXT(Instance, &DebugCreateInfo, nullptr, &DebugMessenger) != VK_SUCCESS)
    {
        spdlog::error("创建Vulkan调试消息失败！");
    }
}

void FVulkanRenderer::CreateLogicalDevice()
{
    // 指定创建的队列
    QueueFamilyIndices FamilyIndices = FindQueueFamilies(PhysicalDevice);
    if (!FamilyIndices.IsComplete())
    {
        spdlog::error("需求的队列族索引不完整，无法创建逻辑设备！");
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

    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR bufferDeviceAddressFeatures{};
    bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE; // 明确启用设备地址特性

    // 指定使用的设备特性
    VkPhysicalDeviceFeatures DeviceFeatures = {};
    DeviceFeatures.geometryShader = VK_TRUE; // 启用几何着色器
    DeviceFeatures.fillModeNonSolid = VK_TRUE; // 启用非实心填充模式
    DeviceFeatures.samplerAnisotropy = VK_TRUE; // 启用各向异性过滤

    // 创建逻辑设备
    VkDeviceCreateInfo DeviceCreateInfo = {};
    DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());
    DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
    DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;
    DeviceCreateInfo.pNext = &bufferDeviceAddressFeatures; // 链接设备特性结构体

    DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
    DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();
    if (EnableValidationLayers)
    {
        DeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
        DeviceCreateInfo.ppEnabledLayerNames = ValidationLayers.data();
    }
    else
    {
        DeviceCreateInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &LogicalDevice) != VK_SUCCESS)
    {
        spdlog::error("创建逻辑设备失败！");
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
        spdlog::error("交换链支持详情不可用！");
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
        spdlog::error("创建交换链失败！");
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
    for (size_t I = 0; I < SwapChainImages.size(); ++I)
    {
        SwapChainImageViews[I] = CreateImageView(SwapChainImages[I], SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
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

    VkAttachmentDescription DepthAttachment = {};
    DepthAttachment.format = FindDepthFormat();
    DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 使用1倍采样
    DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // 清除颜色附件
    DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 不存储深度附件
    DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // 忽略模板附件加载
    DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 忽略模板附件存储
    DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // 初始布局为未定义
    DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // 最终布局为深度模板附件
    VkAttachmentReference DepthAttachmentRef = {};
    DepthAttachmentRef.attachment = 1; // 附件索引
    DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription Subpass = {};
    Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // 图形管线绑定点
    Subpass.colorAttachmentCount = 1;
    Subpass.pColorAttachments = &ColorAttachmentRef;
    Subpass.pDepthStencilAttachment = &DepthAttachmentRef;
    VkSubpassDependency Dependency = {};
    Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    Dependency.dstSubpass = 0;
    Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    Dependency.srcAccessMask = 0;
    Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::array Attachments = { ColorAttachment, DepthAttachment };
    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
    RenderPassInfo.pAttachments = Attachments.data();
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

    ShaderModules.push_back(FragmentShaderModule);
    ShaderModules.push_back(VertexShaderModule);


    auto AttributeDescriptions = GetAttributeDescriptions<Assets::BaseMesh>();
    auto BindingBindingDescriptions = GetBindingDescriptions<Assets::BaseMesh>();
    // 顶点输入状态
    VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
    VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(BindingBindingDescriptions.size()); // 如果没有顶点绑定描述符
    VertexInputInfo.pVertexBindingDescriptions = BindingBindingDescriptions.data(); // 没有顶点绑定描述符
    VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(AttributeDescriptions.size()); // 如果没有顶点属性描述符
    VertexInputInfo.pVertexAttributeDescriptions = AttributeDescriptions.data(); // 没有顶点属性描述符
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
    Scissor.offset = {.x = 0, .y = 0 };
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
    Rasterizer.cullMode = VK_CULL_MODE_NONE; // 背面剔除
    Rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // 顺时针为前面
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
    VkPipelineDepthStencilStateCreateInfo DepthStencil = {};
    DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencil.depthTestEnable = VK_TRUE; // 启用深度测试
    DepthStencil.depthWriteEnable = VK_TRUE; // 启用深度写入
    DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS; // 深度比较操作
    DepthStencil.depthBoundsTestEnable = VK_FALSE; // 不启用深度边界
    DepthStencil.stencilTestEnable = VK_FALSE;
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
    PipelineLayoutInfo.setLayoutCount = 1;
    PipelineLayoutInfo.pSetLayouts = &DescriptorSetLayout;
    PipelineLayoutInfo.pushConstantRangeCount = 0; // 如果没有推送常量范围
    PipelineLayoutInfo.pPushConstantRanges = nullptr; // 没有推送常量范围
    if (vkCreatePipelineLayout(LogicalDevice, &PipelineLayoutInfo, nullptr, &PipelineLayout) != VK_SUCCESS)
    {
        spdlog::error("创建管线布局失败！");
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo PipelineCreateInfo = {};
    PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.stageCount = 2;
    PipelineCreateInfo.pStages = ShaderStages;
    PipelineCreateInfo.pVertexInputState = &VertexInputInfo;
    PipelineCreateInfo.pInputAssemblyState = &InputAssembly;
    PipelineCreateInfo.pViewportState = &ViewportState;
    PipelineCreateInfo.pRasterizationState = &Rasterizer;
    PipelineCreateInfo.pMultisampleState = &Multisampling;
    PipelineCreateInfo.pDepthStencilState = &DepthStencil; // 如果没有深度和模板测试
    PipelineCreateInfo.pColorBlendState = &ColorBlending;
    PipelineCreateInfo.pDynamicState = &DynamicState;
    PipelineCreateInfo.layout = PipelineLayout;
    PipelineCreateInfo.renderPass = RenderPass;
    PipelineCreateInfo.subpass = 0; // 使用第一个子通道
    PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // 不使用基础管线
    PipelineCreateInfo.basePipelineIndex = -1; // 不使用基础管线索引

    if (vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, 1, &PipelineCreateInfo,
        nullptr, &GraphicsPipeline)!= VK_SUCCESS)
    {
        spdlog::error("创建图形管线失败！");
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
}

void FVulkanRenderer::CreateCommandPool()
{
    QueueFamilyIndices FamilyIndices = FindQueueFamilies(PhysicalDevice);
    if (!FamilyIndices.IsComplete())
    {
        spdlog::error("需求的队列族索引不完整，无法创建命令池！");
        throw std::runtime_error("Queue family indices are not complete!");
    }
    VkCommandPoolCreateInfo PoolCreateInfo = {};
    PoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    PoolCreateInfo.flags = 0;
    PoolCreateInfo.queueFamilyIndex = FamilyIndices.GraphicsFamily.value(); // 使用图形队列族
    if (vkCreateCommandPool(LogicalDevice, &PoolCreateInfo, nullptr, &CommandPool) != VK_SUCCESS)
    {
        spdlog::error("创建命令池失败！");
        throw std::runtime_error("Failed to create command pool!");
    }
}

void FVulkanRenderer::CreateDepthResources()
{
    VkFormat DepthFormat = FindDepthFormat();
    VMAImgCreateInfo CreateInfo = {};
    CreateInfo.Width = SwapChainExtent.width;
    CreateInfo.Height = SwapChainExtent.height;
    CreateInfo.Tiling = VK_IMAGE_TILING_OPTIMAL;
    CreateInfo.Format = DepthFormat;
    CreateInfo.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    CreateInfo.MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    CreateInfo.AllocationCreateFlags = 0;
    DepthImageCache = CreateImage(MemoryAllocator, CreateInfo);
    DepthImageView = CreateImageView(DepthImageCache.ImageHandle, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    TransitionImageLayout(DepthImageCache.ImageHandle, DepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void FVulkanRenderer::CreateTextureImage()
{
    auto ImportInfo = FImageImporter::ImportImage("Assets/Models/viking_room.png");
    if (ImportInfo.has_value())
    {
        VkDeviceSize ImageSize = static_cast<VkDeviceSize>(ImportInfo->Width) * ImportInfo->Height * 4;

        auto BufferCache = CreateBufferPack(ImageSize, MemoryAllocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY, 0);

        void* Data;
        vmaMapMemory(MemoryAllocator, BufferCache[0].Allocation, &Data);
        std::memcpy(Data, ImportInfo->Pixels, ImageSize);
        vmaUnmapMemory(MemoryAllocator, BufferCache[0].Allocation);
        FImageImporter::FreeImage(ImportInfo.value());

        VMAImgCreateInfo CreateInfo = {};
        CreateInfo.Width = ImportInfo->Width;
        CreateInfo.Height = ImportInfo->Height;
        CreateInfo.Format = VK_FORMAT_R8G8B8A8_UNORM;
        CreateInfo.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        CreateInfo.MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
        CreateInfo.AllocationCreateFlags = 0;

        TextureImageCache = CreateImage(MemoryAllocator, CreateInfo);
        TransitionImageLayout(TextureImageCache.ImageHandle, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        CopyBufferToImage(BufferCache[0], TextureImageCache);
        // 为了了在着色器中读取纹理，我们需要将图像布局转换为着色器读取专用
        TransitionImageLayout(TextureImageCache.ImageHandle, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // 清理临时缓冲区
        vmaDestroyBuffer(MemoryAllocator, BufferCache[0].BufferHandle, BufferCache[0].Allocation);
    }
}

void FVulkanRenderer::CreateTextureImageView()
{
    TextureImageView = CreateImageView(TextureImageCache.ImageHandle, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

void FVulkanRenderer::CreateTextureSampler()
{
    VkSamplerCreateInfo SamplerCreateInfo = {};
    SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCreateInfo.magFilter = VK_FILTER_LINEAR; // 放大过滤器
    SamplerCreateInfo.minFilter = VK_FILTER_LINEAR; // 缩小过滤器
    SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // U方向重复
    SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; // V方向重复
    SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // W方向重复
    SamplerCreateInfo.anisotropyEnable = VK_TRUE; // 启用各向异性过滤
    SamplerCreateInfo.maxAnisotropy = 16; // 最大各向异性等级
    SamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // 边界颜色
    SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE; // 使用归一化坐标
    SamplerCreateInfo.compareEnable = VK_FALSE; // 不启用比较操作
    SamplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS; // 比较操作
    SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // 线性mipmap过滤
    SamplerCreateInfo.mipLodBias = 0.0f; // LOD偏差
    SamplerCreateInfo.minLod = 0.0f; // 最小LOD
    SamplerCreateInfo.maxLod = 0.0f; // 最大LOD
    if (vkCreateSampler(LogicalDevice, &SamplerCreateInfo, nullptr, &TextureSampler) != VK_SUCCESS)
    {
        spdlog::error("创建纹理采样器失败！");
    }
}

void FVulkanRenderer::CreateVertexBuffers()
{
    auto Model = FModelImporter::ImporterModel("Assets/Models/viking_room.obj");

    if (Model == nullptr)return;;

    LoadedModel = Model;
    const auto& Mesh = Model->MeshData;

    // 创建临时缓冲区
    StagingBufferCaches = CreateBuffer(Mesh, MemoryAllocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY, 0);

    // 写入顶点数据
    void* Data = nullptr;
    std::size_t DataSize = StagingBufferCaches[0].BufferSize;
    vmaMapMemory(MemoryAllocator, StagingBufferCaches[0].Allocation, &Data);
    std::memcpy(Data, (void*)Mesh.Positions.data(), DataSize);
    vmaUnmapMemory(MemoryAllocator, StagingBufferCaches[0].Allocation);
    Data = nullptr;
    DataSize = StagingBufferCaches[1].BufferSize;
    vmaMapMemory(MemoryAllocator, StagingBufferCaches[1].Allocation, &Data);
    std::memcpy(Data, (void*)Mesh.Color.data(), DataSize);
    vmaUnmapMemory(MemoryAllocator, StagingBufferCaches[1].Allocation);
    Data = nullptr;
    DataSize = StagingBufferCaches[2].BufferSize;
    vmaMapMemory(MemoryAllocator, StagingBufferCaches[2].Allocation, &Data);
    std::memcpy(Data, (void*)Mesh.TexCoord.data(), DataSize);
    vmaUnmapMemory(MemoryAllocator, StagingBufferCaches[2].Allocation);

    // 创建顶点缓冲区
    VertexBufferCaches = CreateBuffer(Mesh, MemoryAllocator,
        static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
        VMA_MEMORY_USAGE_GPU_ONLY, 0);

    CopyBuffer(StagingBufferCaches, VertexBufferCaches);
    // 销毁临时缓冲区
    for (const auto& BufferCache : StagingBufferCaches)
    {
        vmaDestroyBuffer(MemoryAllocator, BufferCache.BufferHandle, BufferCache.Allocation);
    }
}

void FVulkanRenderer::CreateIndexBuffer()
{
    //// 创建临时缓冲区
    //StagingBufferCaches = CreateBuffer(Assets::TestTriangleMeshIndices, MemoryAllocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    //    VMA_MEMORY_USAGE_CPU_ONLY, 0);
    //// 写入顶点索引数据
    //void* Data = nullptr;
    //std::size_t DataSize = StagingBufferCaches[0].BufferSize;
    //vmaMapMemory(MemoryAllocator, StagingBufferCaches[0].Allocation, &Data);
    //std::memcpy(Data, (void*)Assets::TestTriangleMeshIndices.Indices.data(), DataSize);
    //vmaUnmapMemory(MemoryAllocator, StagingBufferCaches[0].Allocation);

    //IndexBufferCaches = CreateBuffer(Assets::TestTriangleMeshIndices, MemoryAllocator, 
    //    static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
    //    VMA_MEMORY_USAGE_GPU_ONLY, 0);
    //CopyBuffer(StagingBufferCaches, IndexBufferCaches);
    //// 销毁临时缓冲区
    //for (const auto& BufferCache : StagingBufferCaches)
    //{
    //    vmaDestroyBuffer(MemoryAllocator, BufferCache.Buffer, BufferCache.Allocation);
    //}
}


void FVulkanRenderer::CreateConstantBuffer()
{
    ConstantBufferCaches = CreateBufferPack(sizeof(Assets::TestTriangleMeshUniformBufferObject), MemoryAllocator,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, 0);
}

void FVulkanRenderer::UpdateBuffer(const double Time)
{
    Math::Vec3 EulerAngle(0.0f, 0.0f, static_cast<float>(Time) * 90.0f);
    Assets::TestTriangleMeshUniformBufferObject.Model.block<3, 3>(0, 0)
        = Math::ToQuaternion(EulerAngle).normalized().toRotationMatrix();
    Assets::TestTriangleMeshUniformBufferObject.View = Math::LookAt(Math::Vec3(2.0f, 2.0f, 2.0f), 
                                                                  Math::Vec3(0.0f, 0.0f, 0.0f), 
                                                                     Math::Vec3(0.0f, 0.0f, 1.0f));
    Assets::TestTriangleMeshUniformBufferObject.Projection
        = Math::Perspective(Math::ToRadians(45.f), SwapChainExtent.width / (float)SwapChainExtent.height, 0.1f, 10.0f);

    Assets::TestTriangleMeshUniformBufferObject.Projection(1, 1) *= -1; //Vulkan 的NDC是向下

    void* Data = nullptr;
    vmaMapMemory(MemoryAllocator, ConstantBufferCaches[0].Allocation, &Data);
    std::memcpy(Data, &Assets::TestTriangleMeshUniformBufferObject, sizeof(Assets::TestTriangleMeshUniformBufferObject));
    vmaUnmapMemory(MemoryAllocator, ConstantBufferCaches[0].Allocation);
}

void FVulkanRenderer::CreateDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> PoolSizes = {};
    PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    PoolSizes[0].descriptorCount = 1; // 假设只需要一个描述符
    PoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    PoolSizes[1].descriptorCount = 1; // 假设只需要一个

    VkDescriptorPoolCreateInfo PoolCreateInfo = {};
    PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolCreateInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
    PoolCreateInfo.pPoolSizes = PoolSizes.data();
    PoolCreateInfo.maxSets = 1; // 假设只需要一个描述符
    if (vkCreateDescriptorPool(LogicalDevice, &PoolCreateInfo, nullptr, &DescriptorPool) != VK_SUCCESS)
    {
        spdlog::error("创建描述符池失败！");
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void FVulkanRenderer::CreateDescriptorSet()
{
    VkDescriptorSetAllocateInfo AllocateInfo = {};
    AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    AllocateInfo.descriptorPool = DescriptorPool;
    AllocateInfo.descriptorSetCount = 1; // 假设只需要一个描述符
    AllocateInfo.pSetLayouts = &DescriptorSetLayout;
    if (vkAllocateDescriptorSets(LogicalDevice, &AllocateInfo, &DescriptorSet) != VK_SUCCESS)
    {
        spdlog::error("分配描述符集失败！");
        throw std::runtime_error("Failed to allocate descriptor set!");
    }
    VkDescriptorBufferInfo BufferInfo = {};
    BufferInfo.buffer = ConstantBufferCaches[0].BufferHandle;
    BufferInfo.offset = 0;
    BufferInfo.range = sizeof(Assets::TestTriangleMeshUniformBufferObject);

    VkDescriptorImageInfo ImageInfo = {};
    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImageInfo.imageView = TextureImageView;
    ImageInfo.sampler = TextureSampler;

    std::array<VkWriteDescriptorSet, 2> DescriptorWrites = {};
    DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrites[0].dstSet = DescriptorSet;
    DescriptorWrites[0].dstBinding = 0;
    DescriptorWrites[0].dstArrayElement = 0;
    DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DescriptorWrites[0].descriptorCount = 1;
    DescriptorWrites[0].pBufferInfo = &BufferInfo;

    DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrites[1].dstSet = DescriptorSet;
    DescriptorWrites[1].dstBinding = 1;
    DescriptorWrites[1].dstArrayElement = 0;
    DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    DescriptorWrites[1].descriptorCount = 1; 
    DescriptorWrites[1].pImageInfo = &ImageInfo;

    vkUpdateDescriptorSets(LogicalDevice, static_cast<uint32_t>(DescriptorWrites.size()), DescriptorWrites.data(), 0, nullptr);
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
        spdlog::error("分配命令缓冲失败！");
        throw std::runtime_error("Failed to allocate command buffers!");
    }
    for (size_t Idx = 0; Idx < CommandBuffers.size(); ++Idx)
    {
        VkCommandBufferBeginInfo BeginInfo = {};
        BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // 可选标志
        BeginInfo.pInheritanceInfo = nullptr; // 可选继承信息
        if (vkBeginCommandBuffer(CommandBuffers[Idx], &BeginInfo) != VK_SUCCESS)
        {
            spdlog::error("开始录制命令缓冲失败！");
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        std::array<VkClearValue, 2> ClearValues = {};
        ClearValues[0].color = {0.f, 0.f, 0.f, 1.f};
        ClearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo RenderPassInfo = {};
        RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassInfo.renderPass = RenderPass;
        RenderPassInfo.framebuffer = SwapChainFramebuffers[Idx];
        RenderPassInfo.renderArea.offset = {.x = 0, .y = 0 };
        RenderPassInfo.renderArea.extent = SwapChainExtent;
        RenderPassInfo.clearValueCount = static_cast<uint32_t>(ClearValues.size());
        RenderPassInfo.pClearValues = ClearValues.data();
        vkCmdBindPipeline(CommandBuffers[Idx], VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);

        // 绑定顶点缓冲区
        std::vector<VkBuffer> VertexBuffers(VertexBufferCaches.size());
        for (int I = 0; I < VertexBuffers.size(); ++I)VertexBuffers[I] = VertexBufferCaches[I].BufferHandle;
        VkDeviceSize OffSets[] = {0, 0, 0};
        vkCmdBindVertexBuffers(CommandBuffers[Idx], 0, static_cast<uint32_t>(VertexBuffers.size()), VertexBuffers.data(), OffSets);
        //vkCmdBindIndexBuffer(CommandBuffers[Idx], IndexBufferCaches[0].Buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(CommandBuffers[Idx], VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescriptorSet, 0, nullptr);
        vkCmdBeginRenderPass(CommandBuffers[Idx], &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDraw(CommandBuffers[Idx], LoadedModel->MeshData.Positions.size(), 1, 0, 0); // 绘制一个三角形
        vkCmdEndRenderPass(CommandBuffers[Idx]);
        if (vkEndCommandBuffer(CommandBuffers[Idx]) != VK_SUCCESS)
        {
            spdlog::error("结束录制命令缓冲失败！");
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
        spdlog::error("创建信号量失败！");
        throw std::runtime_error("Failed to create semaphores!");
    }
}

void FVulkanRenderer::RecreateSwapChain(int Width, int Height)
{
    vkDeviceWaitIdle(LogicalDevice);

    CleanupSwapChain();

    CreateSwapChain(Width, Height);
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateDepthResources();
    CreateFramebuffers();
    CreateCommandBuffers();
}

void FVulkanRenderer::CleanupSwapChain()
{
    vkDestroyImageView(LogicalDevice, DepthImageView, nullptr);
    vmaDestroyImage(MemoryAllocator, DepthImageCache.ImageHandle, DepthImageCache.Allocation);

    vkFreeCommandBuffers(LogicalDevice, CommandPool, static_cast<uint32_t>(CommandBuffers.size()), CommandBuffers.data());
    for (auto Framebuffer : SwapChainFramebuffers)
    {
        vkDestroyFramebuffer(LogicalDevice, Framebuffer, nullptr);
    }

    vkDestroyPipeline(LogicalDevice, GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);
    vkDestroyRenderPass(LogicalDevice, RenderPass, nullptr);

    for (auto ImageView : SwapChainImageViews)
    {
        vkDestroyImageView(LogicalDevice, ImageView, nullptr);
    }

    vkDestroySwapchainKHR(LogicalDevice, SwapChain, nullptr);
}

void FVulkanRenderer::CreateFramebuffers()
{
    SwapChainFramebuffers.resize(SwapChainImageViews.size());

    for (size_t i = 0; i < SwapChainImageViews.size(); ++i)
    {
        std::array Attachments = { SwapChainImageViews[i], DepthImageView};

        VkFramebufferCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        CreateInfo.renderPass = RenderPass;
        CreateInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
        CreateInfo.pAttachments = Attachments.data();
        CreateInfo.width = SwapChainExtent.width;
        CreateInfo.height = SwapChainExtent.height;
        CreateInfo.layers = 1;
        if (vkCreateFramebuffer(LogicalDevice, &CreateInfo, nullptr, &SwapChainFramebuffers[i]) != VK_SUCCESS)
        {
            spdlog::error("创建帧缓冲失败！");
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

void FVulkanRenderer::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding CBufferLayoutBinding = {};
    CBufferLayoutBinding.binding = 0;
    CBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    CBufferLayoutBinding.descriptorCount = 1;
    CBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    CBufferLayoutBinding.pImmutableSamplers = nullptr; // 可选

    VkDescriptorSetLayoutBinding SamplerLayoutBinding = {};
    SamplerLayoutBinding.binding = 1;
    SamplerLayoutBinding.descriptorCount = 1;
    SamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    SamplerLayoutBinding.pImmutableSamplers = nullptr;
    SamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array Bindings = { CBufferLayoutBinding, SamplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = {};
    LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LayoutCreateInfo.bindingCount = static_cast<uint32_t>(Bindings.size());
    LayoutCreateInfo.pBindings = Bindings.data();

    if (vkCreateDescriptorSetLayout(LogicalDevice, &LayoutCreateInfo, nullptr, &DescriptorSetLayout) != VK_SUCCESS) 
    {
        spdlog::error("创建描述符集布局失败！");
        throw std::runtime_error("failed to create descriptor set layout!");
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
    VulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
    VulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;
    VulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
    VulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
    VulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
    VulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
    VulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
    VulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
    VulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2;
    VulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
    VulkanFunctions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
    VulkanFunctions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    VulkanFunctions.vkGetMemoryWin32HandleKHR = vkGetMemoryWin32HandleKHR;
#endif

    VmaAllocatorCreateInfo AllocatorCreateInfo = {};
    AllocatorCreateInfo.vulkanApiVersion = USE_VULKAN_VERSION;
    AllocatorCreateInfo.physicalDevice = PhysicalDevice;
    AllocatorCreateInfo.device = LogicalDevice;
    AllocatorCreateInfo.instance = Instance;
    AllocatorCreateInfo.pVulkanFunctions = &VulkanFunctions;
    AllocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // 加上这个才能获取地址

    if (vmaCreateAllocator(&AllocatorCreateInfo, &MemoryAllocator) != VK_SUCCESS)
    {
        spdlog::error("创建VMA分配器失败！");
    }
}

VkCommandBuffer FVulkanRenderer::BeginSingleTimeCommands() const
{
    VkCommandBufferAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    AllocInfo.commandPool = CommandPool;
    AllocInfo.commandBufferCount = 1;
    VkCommandBuffer CommandBuffer;
    vkAllocateCommandBuffers(LogicalDevice, &AllocInfo, &CommandBuffer);

    VkCommandBufferBeginInfo BeginInfo = {};
    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // 告知Vulkan驱动程序使用VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT是一个好的习惯
    BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(CommandBuffer, &BeginInfo);

    return CommandBuffer;
}

void FVulkanRenderer::EndSingleTimeCommands(const VkCommandBuffer CommandBuffer) const
{
    vkEndCommandBuffer(CommandBuffer);

    // 提交并等待复制完成
    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CommandBuffer;
    vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(GraphicsQueue);

    // 清理临时资源
    vkFreeCommandBuffers(LogicalDevice, CommandPool, 1, &CommandBuffer);
}

void FVulkanRenderer::CopyBuffer(const std::vector<VMABufferCache>& SrcBuffer, const std::vector<VMABufferCache>& DstBuffer)
{
    assert(SrcBuffer.size() == DstBuffer.size());

    // 创建一个临时命令缓冲区来执行复制操作
    VkCommandBuffer CommandBuffer = BeginSingleTimeCommands();
    for (size_t Idx = 0; Idx < SrcBuffer.size(); ++Idx)
    {
        const VMABufferCache& SrcBufferCache = SrcBuffer[Idx];
        const VMABufferCache& DstBufferCache = DstBuffer[Idx];
        VkBufferCopy CopyRegion = {};
        CopyRegion.srcOffset = 0;
        CopyRegion.dstOffset = 0;
        CopyRegion.size = SrcBufferCache.BufferSize;
        vkCmdCopyBuffer(CommandBuffer, SrcBufferCache.BufferHandle, DstBufferCache.BufferHandle, 1, &CopyRegion);
    }
    EndSingleTimeCommands(CommandBuffer);
}

void FVulkanRenderer::CopyBufferToImage(const VMABufferCache& Buffer, const VMAImageCache& Image) const
{
    const VkCommandBuffer CommandBuffer = BeginSingleTimeCommands();
    VkBufferImageCopy Region = {};
    Region.bufferOffset = 0;
    Region.bufferRowLength = 0; // 0表示紧密打包
    Region.bufferImageHeight = 0; // 0表示紧密打包
    Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Region.imageSubresource.mipLevel = 0;
    Region.imageSubresource.baseArrayLayer = 0;
    Region.imageSubresource.layerCount = 1;
    Region.imageOffset = {.x = 0, .y = 0, .z = 0};
    Region.imageExtent = {.width = Image.Width, .height = Image.Height, .depth = 1 };
    vkCmdCopyBufferToImage(CommandBuffer, Buffer.BufferHandle, Image.ImageHandle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
    EndSingleTimeCommands(CommandBuffer);
}

void FVulkanRenderer::TransitionImageLayout(VkImage Image, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout) const
{
    VkCommandBuffer CommandBuffer = BeginSingleTimeCommands();

    VkPipelineStageFlags SourceStage;
    VkPipelineStageFlags DestinationStage;

    VkImageMemoryBarrier Barrier = {};
    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.oldLayout = OldLayout;
    Barrier.newLayout = NewLayout;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // VK_QUEUE_FAMILY_IGNORED不是默认值，需要显式指定
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.image = Image;
    Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Barrier.subresourceRange.baseMipLevel = 0;
    Barrier.subresourceRange.levelCount = 1;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount = 1;

    if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        SourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (HasStencilComponent(Format)) 
        {
            Barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else 
    {
        spdlog::error("不支持的图像布局转换！");
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(CommandBuffer, SourceStage, DestinationStage, 0, 0, nullptr, 0, nullptr, 1, &Barrier);

    EndSingleTimeCommands(CommandBuffer);
}

VkImageView FVulkanRenderer::CreateImageView(VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags) const
{
    VkImageViewCreateInfo ViewInfo = {};
    ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ViewInfo.image = Image;
    ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ViewInfo.format = Format;
    ViewInfo.subresourceRange.aspectMask = AspectFlags;
    ViewInfo.subresourceRange.baseMipLevel = 0;
    ViewInfo.subresourceRange.levelCount = 1;
    ViewInfo.subresourceRange.baseArrayLayer = 0;
    ViewInfo.subresourceRange.layerCount = 1;

    VkImageView ImageView;
    if (vkCreateImageView(LogicalDevice, &ViewInfo, nullptr, &ImageView) != VK_SUCCESS) 
    {
        spdlog::error("创建纹理图像视图失败！");
    }

    return ImageView;
}

VkFormat FVulkanRenderer::FindSupportedFormat(const std::vector<VkFormat>& Candidates, VkImageTiling Tiling,
    VkFormatFeatureFlags Features)
{
    for (VkFormat Format : Candidates)
    {
        VkFormatProperties Props;
        vkGetPhysicalDeviceFormatProperties(PhysicalDevice, Format, &Props);
        if (Tiling == VK_IMAGE_TILING_LINEAR && (Props.linearTilingFeatures & Features) == Features)
        {
            return Format;
        }
        if (Tiling == VK_IMAGE_TILING_OPTIMAL && (Props.optimalTilingFeatures & Features) == Features)
        {
            return Format;
        }
    }
    spdlog::error("找不到支持的格式！");
    throw std::runtime_error("failed to find supported format!");
}
