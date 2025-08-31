#pragma once

#include <cstdint>

namespace SilverBell::Application
{
    class FConfig
    {
    public:
        FConfig() = delete;

        enum class ERendererType : std::uint8_t
        {
            Vulkan,
            OpenGL,
            DirectX,
            Metal
        };

        static ERendererType RendererType;
    };

}
