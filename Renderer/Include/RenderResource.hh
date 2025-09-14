#pragma once

#include <spdlog/spdlog.h>

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <ylt/reflection/member_value.hpp>

namespace SilverBell::Renderer
{
    // C++ 17 inline变量
    inline const std::unordered_map<std::size_t, int> VkFormatMap =
    {
        // double
        { sizeof(float)    , VK_FORMAT_R32_SFLOAT},
        { sizeof(float) * 2, VK_FORMAT_R32G32_SFLOAT },
        { sizeof(float) * 3, VK_FORMAT_R32G32B32_SFLOAT },
        { sizeof(float) * 4, VK_FORMAT_R32G32B32A32_SFLOAT },

        // double
        { sizeof(double)    , VK_FORMAT_R64_SFLOAT},
        { sizeof(double) * 2, VK_FORMAT_R64G64_SFLOAT },
        { sizeof(double) * 3, VK_FORMAT_R64G64B64_SFLOAT },
        { sizeof(double) * 4, VK_FORMAT_R64G64B64A64_SFLOAT },
    };

    // 获取顶点属性描述
    template<typename T, std::size_t I = ylt::reflection::members_count_v<T>>
    [[nodiscard]] constexpr std::array<VkVertexInputAttributeDescription, I> GetAttributeDescriptions(const T& iMesh)
    {
        std::array<VkVertexInputAttributeDescription, I> AttributeDescriptions = {};

        ylt::reflection::for_each(iMesh,[&AttributeDescriptions](auto& Field, auto Name, auto Index)
        {
            using MemberType = std::remove_cvref_t<decltype(Field)>;

            size_t ValueSize;
            // C++ 20 requires 语法
            if constexpr (requires { typename MemberType::value_type; })
            {
                ValueSize = sizeof(typename MemberType::value_type);
            }
            else
            {
                ValueSize = sizeof(MemberType);
            }

            AttributeDescriptions[Index].binding = Index;
            AttributeDescriptions[Index].location = Index;
            auto Finder = VkFormatMap.find(ValueSize);
            if (Finder == VkFormatMap.end())
            {
                spdlog::warn("不支持的顶点属性格式！");
            }
            AttributeDescriptions[Index].format = Finder != VkFormatMap.end()
                ? static_cast<VkFormat>(Finder->second) : VK_FORMAT_R32G32B32A32_SFLOAT; // TODO: 根据类型动态设置
            // 由于顶点属性是 分离存储 (Separated)，每个属性都是单独的数组存储，所以offset就是0
            AttributeDescriptions[Index].offset = 0;
        });

        return AttributeDescriptions;
    }

    // 获取顶点属性描述（不传参版本，需要类型T可默认构造）
    template<typename T, std::size_t I = ylt::reflection::members_count_v<T>>
    requires std::default_initializable<T>
    [[nodiscard]] constexpr std::array<VkVertexInputAttributeDescription, I> GetAttributeDescriptions()
    {
        return GetAttributeDescriptions(T{});
    }

    // 获取顶点绑定描述
    template<typename T, std::size_t I = ylt::reflection::members_count_v<T>>
    [[nodiscard]] std::array<VkVertexInputBindingDescription, I> GetBindingDescriptions(const T& iMesh)
    {
        std::array<VkVertexInputBindingDescription, I> BindingDescription = {};
        ylt::reflection::for_each(iMesh, [&BindingDescription](auto& Field, auto Name, auto Index)
        {
            using MemberType = std::remove_cvref_t<decltype(Field)>;

            size_t ValueSize;
            if constexpr (requires { typename MemberType::value_type; })
            {
                ValueSize = sizeof(typename MemberType::value_type);
            }
            else
            {
                ValueSize = sizeof(MemberType);
            }

            BindingDescription[Index].binding = Index;
            BindingDescription[Index].stride = ValueSize;
            BindingDescription[Index].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        });

        return BindingDescription;
    }

    // 获取顶点绑定描述（不传参版本，需要类型T可默认构造）
    template<typename T, std::size_t I = ylt::reflection::members_count_v<T>>
    requires std::default_initializable<T>
    [[nodiscard]] std::array<VkVertexInputBindingDescription, I> GetBindingDescriptions()
    {
        return GetBindingDescriptions(T{});
    }

    struct VkBufferCache
    {
        VkDeviceSize BufferSize = 0;
        VkBuffer Buffer = VK_NULL_HANDLE;
        VmaAllocation Allocation = VK_NULL_HANDLE;
        VmaAllocationInfo AllocationInfo = {};
    };



    // 创建缓冲区，返回每个成员对应的缓冲区数组
    template<typename T, std::size_t I = ylt::reflection::members_count_v<T>>
    [[nodiscard]] std::vector<VkBufferCache> CreateBuffer(const T& iDataSource,
                                                         VmaAllocator MemoryAllocator,   
                                                         VkBufferUsageFlagBits VkBufferUsageFlagBits,
                                                         VmaMemoryUsage VmaUsageBits, 
                                                         VmaAllocationCreateFlags VmaCreateFlagsBits)
    {
        std::vector<VkBufferCache> BufferCaches;
        BufferCaches.resize(I);

        ylt::reflection::for_each(iDataSource, 
            [&BufferCaches, &MemoryAllocator, VkBufferUsageFlagBits, VmaUsageBits, VmaCreateFlagsBits]
            (auto& Field, auto Name, auto Index)
            {
                using MemberType = std::remove_cvref_t<decltype(Field)>;

                size_t ValueSize;
                if constexpr (requires { typename MemberType::value_type; Field.size(); })
                {
                    ValueSize = sizeof(typename MemberType::value_type) * Field.size();
                }
                else
                {
                    ValueSize = sizeof(MemberType);
                }

                VkBufferCreateInfo BufferInfo = {};
                BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                BufferInfo.size = ValueSize;
                BufferInfo.usage = VkBufferUsageFlagBits;
                BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                BufferInfo.flags = 0;

                VmaAllocationCreateInfo AllocCreateInfo = {};
                AllocCreateInfo.usage = VmaUsageBits;
                AllocCreateInfo.flags = VmaCreateFlagsBits;

                VkBufferCache& BufferCache = BufferCaches[Index];
                BufferCache.BufferSize = ValueSize;
                if (vmaCreateBuffer(MemoryAllocator,
                    &BufferInfo,
                    &AllocCreateInfo,
                    &BufferCache.Buffer,
                    &BufferCache.Allocation,
                    &BufferCache.AllocationInfo) != VK_SUCCESS)
                {
                    spdlog::error("创建缓冲区失败！");
                    throw std::runtime_error("Failed to create buffer!");
                }
            }
        );

        return BufferCaches;
    }

    [[nodiscard]] __FORCEINLINE std::vector<VkBufferCache> CreateBufferPack(size_t ValueSize,
        VmaAllocator MemoryAllocator,
        VkBufferUsageFlagBits VkBufferUsageFlagBits,
        VmaMemoryUsage VmaUsageBits,
        VmaAllocationCreateFlags VmaCreateFlagsBits)
    {
        std::vector<VkBufferCache> BufferCaches(1);

        VkBufferCreateInfo BufferInfo = {};
        BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufferInfo.size = ValueSize;
        BufferInfo.usage = VkBufferUsageFlagBits;
        BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        BufferInfo.flags = 0;

        VmaAllocationCreateInfo AllocCreateInfo = {};
        AllocCreateInfo.usage = VmaUsageBits;
        AllocCreateInfo.flags = VmaCreateFlagsBits;

        VkBufferCache& BufferCache = BufferCaches[0];
        BufferCache.BufferSize = ValueSize;
        if (vmaCreateBuffer(MemoryAllocator,
            &BufferInfo,
            &AllocCreateInfo,
            &BufferCache.Buffer,
            &BufferCache.Allocation,
            &BufferCache.AllocationInfo) != VK_SUCCESS)
        {
            spdlog::error("创建缓冲区失败！");
            throw std::runtime_error("Failed to create buffer!");
        }
        return BufferCaches;
    }


    struct VMAImgCreateInfo
    {
        int Width = 0;
        int Height = 0;
        VkFormat Format = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags Usage = 0;
        VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
        VmaAllocationCreateFlags AllocationCreateFlags = 0;
    };

    struct VkImageCache
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        VkImage Image = VK_NULL_HANDLE;
        VmaAllocation Allocation = VK_NULL_HANDLE;
        VmaAllocationInfo AllocationInfo = {};
    };

    [[nodiscard]] __FORCEINLINE VkImageCache CreateImage(VmaAllocator MemoryAllocator, const VMAImgCreateInfo& CreateInfo)
    {
        VkImageCache ImageCache;

        VkImageCreateInfo ImageInfo = {};
        ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ImageInfo.imageType = VK_IMAGE_TYPE_2D;
        ImageInfo.extent.width = static_cast<uint32_t>(CreateInfo.Width);
        ImageInfo.extent.height = static_cast<uint32_t>(CreateInfo.Height);
        ImageInfo.extent.depth = 1;
        ImageInfo.mipLevels = 1;
        ImageInfo.arrayLayers = 1;
        ImageInfo.format = CreateInfo.Format;
        ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ImageInfo.usage = CreateInfo.Usage;
        ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        ImageInfo.flags = 0;

        VmaAllocationCreateInfo AllocCreateInfo = {};
        AllocCreateInfo.usage = CreateInfo.MemoryUsage;
        AllocCreateInfo.flags = CreateInfo.AllocationCreateFlags;


        ImageCache.Width = static_cast<uint32_t>(CreateInfo.Width);
        ImageCache.Height = static_cast<uint32_t>(CreateInfo.Height);
        if (vmaCreateImage(MemoryAllocator,
            &ImageInfo,
            &AllocCreateInfo,
            &ImageCache.Image,
            &ImageCache.Allocation,
            &ImageCache.AllocationInfo) != VK_SUCCESS)
        {
            spdlog::error("创建图片失败！");
            throw std::runtime_error("Failed to create Image!");
        }

        return ImageCache;
    }


}