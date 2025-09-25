#pragma once

#include <string>

namespace SilverBell::Renderer
{
    enum class EResourceType : uint8_t
    {
        Texture,
        Buffer,
        // 可扩展更多类型
    };

    enum class EAccessType : uint8_t
    {
        Read,
        Write,
        ReadWrite,
    };

    // 渲染图资源描述
    struct RenderGraphResourceDesc
    {
        std::string Field;         // 资源名称或标识
        EResourceType Type;        // 资源类型
        EAccessType Access;        // 访问类型

        // 隐藏友元模式(Hidden Friend Idiom)
        friend bool operator==(const RenderGraphResourceDesc& L, const RenderGraphResourceDesc& R)
        {
            return L.Field == R.Field && L.Type == R.Type && L.Access == R.Access;
        }
    };


}

// 特化 std::hash 以支持 RenderGraphResourceDesc 作为 unordered_set 的键
template <>
struct std::hash<SilverBell::Renderer::RenderGraphResourceDesc>
{
    std::size_t operator()(const SilverBell::Renderer::RenderGraphResourceDesc& Desc) const noexcept
    {
        std::size_t h1 = std::hash<std::string>{}(Desc.Field);
        std::size_t h2 = std::hash<int>{}(static_cast<int>(Desc.Type));
        std::size_t h3 = std::hash<int>{}(static_cast<int>(Desc.Access));
        // 简单合并哈希
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};