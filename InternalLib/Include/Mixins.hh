#pragma once

namespace SilverBell
{
    // 禁用拷贝语义
    class NonCopyable
    {
    public:
        NonCopyable() = default;

        NonCopyable(const NonCopyable& Other) = delete;
        NonCopyable& operator=(const NonCopyable& Other) = delete;
    };
}
