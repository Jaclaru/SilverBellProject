#pragma once

namespace SilverBell
{
    // 禁用拷贝语义
    class NonCopyable
    {
    public:
        NonCopyable(const NonCopyable& Other) = delete;
        NonCopyable& operator=(const NonCopyable& Other) = delete;

        NonCopyable() = default;
        NonCopyable(NonCopyable&& Other) = default;
        NonCopyable& operator=(NonCopyable&& Other) = default;
    };

    // 禁用移动语义
    class NonMovable
    {
    public:
        NonMovable(NonMovable&& Other) = delete;
        NonMovable& operator=(NonMovable&& Other) = delete;

        NonMovable() = default;
        NonMovable(const NonMovable& Other) = default;
        NonMovable& operator=(const NonMovable& Other) = default;
    };
}
