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

        void CreateDescriptorSetLayout();

        void CreateGraphicsPipeline();

        void CreateCommandPool();

        void CreateTextureImage();

        void CreateTextureImageView();

        void CreateTextureSampler();

        void CreateVertexBuffers();

        void CreateIndexBuffer();

        void CreateConstantBuffer();

        void UpdateBuffer(const double Time);

        void CreateDescriptorPool();

        void CreateDescriptorSet();

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

        VkCommandBuffer BeginSingleTimeCommands() const;

        void EndSingleTimeCommands(VkCommandBuffer CommandBuffer) const;

        void CopyBuffer(const std::vector<VkBufferCache>& SrcBuffer, const std::vector<VkBufferCache>& DstBuffer);
        void CopyBuffer(const VkBufferCache& SrcBuffer, const VkBufferCache& DstBuffer)
        {
            CopyBuffer(std::vector{ SrcBuffer }, std::vector{ DstBuffer });
        }
        void CopyBufferToImage(const VkBufferCache& Buffer, const VkImageCache& Image) const;

        void TransitionImageLayout(VkImage Image, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout) const;

        VkImageView CreateImageView(VkImage Image, VkFormat Format) const;

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
        // 描述符集布局
        VkDescriptorSetLayout DescriptorSetLayout;
        // 描述符池
        VkDescriptorPool DescriptorPool;
        // 描述符集
        VkDescriptorSet DescriptorSet;

        // Vulkan渲染管线布局
        VkPipelineLayout PipelineLayout;
        // Vulkan渲染管线
        VkPipeline GraphicsPipeline;
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
        // 常量缓冲区
        std::vector<VkBufferCache> ConstantBufferCaches;
        // 纹理图像
        VkImageCache TextureImageCache;
        // 纹理图像视图
        VkImageView TextureImageView;
        // 纹理采样器
        VkSampler TextureSampler;

        // VMA内存分配
        VmaAllocator MemoryAllocator;
    };
}

