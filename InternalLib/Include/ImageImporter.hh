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
            int Width = 0;
            int Height = 0;
            int Channels = 0; // 通道数，默认为4
        };

        static std::optional<ImageInfo> ImportImage(std::string_view FilePath);

        static void FreeImage(const ImageInfo& ImageInfo);

    };
}
