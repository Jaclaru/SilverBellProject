#include "ImageImporter.hh"

#define STB_IMAGE_IMPLEMENTATION
#include<stb/stb_image.h>

#include <spdlog/spdlog.h>

using namespace SilverBell;

std::optional<FImageImporter::ImageInfo> FImageImporter::ImportImage(std::string_view FilePath)
{
    ImageInfo Info;
    std::string FullPath = std::string(PROJECT_ROOT_PATH) + std::string(FilePath);
    unsigned char* Data = stbi_load(FullPath.data(), &Info.Width, &Info.Height, &Info.Channels, STBI_rgb_alpha);
    if (!Data)
    {
        spdlog::error("加载图片失败： {}", FullPath);
        spdlog::error("失败原因：{}", stbi_failure_reason());
        return std::nullopt;
    }
    Info.Pixels = Data;
    return Info;
}

void FImageImporter::FreeImage(const ImageInfo& ImageInfo)
{
    if (ImageInfo.Pixels)
    {
        stbi_image_free(ImageInfo.Pixels);
    }
}
