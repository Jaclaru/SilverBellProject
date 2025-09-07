#pragma once

#include "RendererMarco.hh"

#include <optional>
#include <vector>

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include "Mixins.hh"
#include "RenderResource.hh"

namespace SilverBell::Renderer
{
    class RENDERER_API FVulkanRenderer : public NonCopyable
    {
    public:
        FVulkanRenderer();

        ~FVulkanRenderer();

        void SetRequiredInstanceExtensions(const char** Exts, int Len);

        void CleanUp();

        bool DrawFrame();

        void CreateSurface(void* WindowHandle);

        void CreateSwapChain(int Width, int Height);

        struct QueueFamilyIndices
        {
            std::optional<uint32_t> GraphicsFamily;
            std::optional<uint32_t> PresentFamily;

            __FORCEINLINE bool IsComplete() const
            {
                return GraphicsFamily.has_value() && PresentFamily.has_value();
            }
        };

        struct SwapChainSupportDetails
        {
            VkSurfaceCapabilitiesKHR Capabilities = {};
            std::vector<VkSurfaceFormatKHR> Formats;
            std::vector<VkPresentModeKHR> PresentModes;
        };

        void CreateInstance();

        void PickPhysicalDevice();

        void SetupDebugMessenger();

        void CreateLogicalDevice();

        void CreateImageViews();

        void CreateRenderPass();

        void CreateFramebuffers();

        void CreateGraphicsPipeline();

        void CreateCommandPool();

        void CreateVertexBuffers();

        void CreateIndexBuffer();

        void CreateCommandBuffers();

        void CreateSemaphores();

        void RecreateSwapChain(int Width, int Height);

    private:

        void CleanupSwapChain();

        bool IsDeviceSuitable(VkPhysicalDevice Device);

        bool CheckValidationLayerSupport();

        bool CheckDeviceExtensionSupport(VkPhysicalDevice Device);

        SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice Device);

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice Device);

        void CreateMemoryAllocator();

        void CopyBuffer(std::vector<VkBufferCache> SrcBuffer, std::vector<VkBufferCache> DstBuffer);
        void CopyBuffer(VkBufferCache SrcBuffer, VkBufferCache DstBuffer)
        {
            CopyBuffer(std::vector{ SrcBuffer }, std::vector{ DstBuffer });
        }

        // Vulkan实例扩展
        std::vector<const char*> InstanceExtensions;
        // Vulkan实例
        VkInstance Instance;
        // Vulkan调试消息
        VkDebugUtilsMessengerEXT DebugMessenger;
        // Vulkan物理设备
        VkPhysicalDevice PhysicalDevice;
        // Vulkan逻辑设备
        VkDevice LogicalDevice;
        // Vulkan图形队列
        VkQueue GraphicsQueue;
        // Vulkan呈现队列
        VkQueue PresentQueue;
        // Vulkan表面
        VkSurfaceKHR Surface;
        // Vulkan交换链
        VkSwapchainKHR SwapChain;
        // Vulkan交换链图像
        std::vector<VkImage> SwapChainImages;
        // Vulkan交换链图像格式
        VkFormat SwapChainImageFormat;
        // Vulkan交换链范围
        VkExtent2D SwapChainExtent;
        // Vulkan交换链图像视图
        std::vector<VkImageView> SwapChainImageViews;
        // Vulkan渲染通道
        VkRenderPass RenderPass;
        // Vulkan交换链帧缓冲
        std::vector<VkFramebuffer> SwapChainFramebuffers;
        // Vulkan着色器模块
        std::vector<VkShaderModule> ShaderModules;
        // Vulkan渲染管线
        VkPipeline GraphicsPipeline;
        // Vulkan渲染管线布局
        VkPipelineLayout PipelineLayout;
        // Vulkan命令池
        VkCommandPool CommandPool;
        // Vulkan命令缓冲区
        std::vector<VkCommandBuffer> CommandBuffers;
        // 信号量
        VkSemaphore ImageAvailableSemaphore;
        VkSemaphore RenderFinishedSemaphore;
        // 顶点临时缓冲
        std::vector<VkBufferCache> StagingBufferCaches;
        // 顶点缓冲
        std::vector<VkBufferCache> VertexBufferCaches;
        // 顶点索引缓冲
        std::vector<VkBufferCache> IndexBufferCaches;

        // VMA内存分配
        VmaAllocator MemoryAllocator;
    };
}

