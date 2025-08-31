#pragma once

#include <filesystem>
#include <unordered_map>

#include <string>

#include <Volk/volk.h>

namespace SilverBell::Renderer
{
    class FShaderManager
    {
    public:

        static FShaderManager& Instance();

        void CompileHLSL2SPIRV(const std::string& FilePath, const std::string& EntryPoint, const std::string& ShaderModel, uint32_t ShaderType);

        VkShaderModule CreateShaderModule(const std::string& FilePath, VkDevice LogicDevice);

    private:
        struct ShaderModuleCache
        {
            ~ShaderModuleCache()
            {
                for (auto item : Cache)
                {
                    vkDestroyShaderModule(LogicDevice, item.second, nullptr);
                }
            }

            VkDevice LogicDevice;
            // 缓存：key为文件路径，value为VkShaderModule
            std::unordered_map<std::string, VkShaderModule> Cache;
        };

        FShaderManager() = default;
        ~FShaderManager() = default;
        // 禁用拷贝构造和赋值操作
        FShaderManager(const FShaderManager&) = delete;
        FShaderManager& operator=(const FShaderManager&) = delete;
        // 移动构造和赋值操作
        FShaderManager(FShaderManager&&) = delete;
        FShaderManager& operator=(FShaderManager&&) = delete;

        std::unordered_map<VkDevice, ShaderModuleCache> ShaderCache;

    };
}
