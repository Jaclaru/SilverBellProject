#pragma once

#include "RendererMarco.h"
#include "RendererBase.h"

#include <optional>
#include <vector>
#define VOLK_IMPLEMENTATION
#include <Volk/volk.h>

namespace SilverBell::Renderer
{

    class RENDERER_API VulkanRenderer final : public RendererBase
    {
    public:

        VulkanRenderer() : Instance(nullptr) {}

        ~VulkanRenderer() override;

        void Initialize() override;

        void SetSurfaceExt(const char** Exts, int Len) override;

        void CleanUp() override;

    private:

        void CreateInstance();

        void PickPhysicalDevice();

        bool IsDeviceSuitable(VkPhysicalDevice Device);

        bool CheckValidationLayerSupport();

        void SetupDebugMessenger();

        struct QueueFamilyIndices
        {
            std::optional<uint32_t> GraphicsFamily;

            bool IsComplete() const { return GraphicsFamily.has_value(); }
        };

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice Device);

        // Vulkan实例扩展
        std::vector<const char*> InstanceExtensions;
        // Vulkan实例
        VkInstance Instance;
        // Vulkan调试消息
        VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;
        // Vulkan物理设备
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
    };
}

