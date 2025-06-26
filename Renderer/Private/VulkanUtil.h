#pragma once

#include <Volk/volk.h>

#include <vector>

namespace SilverBell::Render
{
    class VulkanUtil
    {
    public:

        // 获取支持的扩展
        auto GetSupportedExtensions()
        {
            uint32_t ExtensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr);
            std::vector<VkExtensionProperties> Extensions(ExtensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, Extensions.data());
            std::vector<const char*> SupportedExtensions;
            for (const auto& Extension : Extensions)
            {
                SupportedExtensions.push_back(Extension.extensionName);
            }
            return SupportedExtensions;
        }

    private:
        VulkanUtil() = default;
        ~VulkanUtil() = default;
    };
}