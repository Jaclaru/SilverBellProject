#pragma once

#include "InternalLibMarco.hh"

#include <string_view>
#include <optional>

namespace SilverBell
{
    class INTERNALLIB_API FImageImporter
    {
    public:

        FImageImporter() = delete;
        ~FImageImporter() = delete;

        struct ImageInfo
        {
            unsigned char* Pixels = nullptr;
            uint32_t Width = 0;
            uint32_t Height = 0;
            uint8_t Channels = 0; // 通道数，默认为4
        };

        static std::optional<ImageInfo> ImportImage(std::string_view FilePath);

        static void FreeImage(ImageInfo& ImageInfo);

    };
}
