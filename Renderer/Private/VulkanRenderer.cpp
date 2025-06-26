#include "VulkanRenderer.h"

#include <iostream>
#include <vector>

using namespace SilverBell::Renderer;

namespace
{
#if VULKAN_DEBUG_ENABLE
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif

    const std::vector<const char*> ValidationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo()
    {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        //debugCreateInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)DebugCallback;

        debugCreateInfo.pfnUserCallback =
            [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData) -> VkBool32
            {
                std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';

                return VK_FALSE;
            };

        return debugCreateInfo;
    }
}

VulkanRenderer::~VulkanRenderer()
{
    CleanUp();
}

void VulkanRenderer::Initialize()
{
    CreateInstance();
    if constexpr (enableValidationLayers)
    {
        SetupDebugMessenger();
    }
    PickPhysicalDevice();
}

void VulkanRenderer::SetSurfaceExt(const char** Exts, int Len)
{
    for (int i = 0; i < Len; ++i)
    {
        InstanceExtensions.push_back(Exts[i]);
    }
}

void VulkanRenderer::CleanUp()
{
    if (DebugMessenger != VK_NULL_HANDLE)
    {
        vkDestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
        DebugMessenger = VK_NULL_HANDLE;
    }

    if (Instance != nullptr)
    {
        vkDestroyInstance(Instance, nullptr);
        Instance = nullptr;
    }

}

void VulkanRenderer::CreateInstance()
{
    if (volkInitialize() != VK_SUCCESS)
    {
        return;
    }

    if constexpr (enableValidationLayers && !CheckValidationLayerSupport())
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

    if (enableValidationLayers)
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

void VulkanRenderer::PickPhysicalDevice()
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

bool VulkanRenderer::IsDeviceSuitable(VkPhysicalDevice Device)
{
    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(Device, &DeviceProperties);
    VkPhysicalDeviceFeatures DeviceFeatures;
    vkGetPhysicalDeviceFeatures(Device, &DeviceFeatures);

    // 检查队列簇
    auto FamilyIndices = FindQueueFamilies(Device);

    // 检查设备是否支持必要的功能
    return (DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) &&
        DeviceFeatures.geometryShader && FamilyIndices.IsComplete(); // 这里可以添加更多的检查条件
}

bool VulkanRenderer::CheckValidationLayerSupport()
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

void VulkanRenderer::SetupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo = GetDebugMessengerCreateInfo();
    if (vkCreateDebugUtilsMessengerEXT(Instance, &DebugCreateInfo, nullptr, &DebugMessenger) != VK_SUCCESS)
    {

    }
}

VulkanRenderer::QueueFamilyIndices VulkanRenderer::FindQueueFamilies(VkPhysicalDevice Device)
{
    QueueFamilyIndices Indices;

    uint32_t QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, QueueFamilies.data());

    int Idx = 0;
    for (const auto& QueueFamily : QueueFamilies)
    {
        if (QueueFamily.queueCount > 0 && QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            Indices.GraphicsFamily = Idx;
        }

        if (Indices.IsComplete())break;
    }

    return Indices;
}
