#pragma once

#include <filesystem>
#include <unordered_map>

#include <string>

#include <Volk/volk.h>

#include <Mixins.hh>
#include <unordered_set>

#include "Shader.hh"

namespace SilverBell::Renderer
{
    class FShaderManager : public NonCopyable
    {
    public:
        static FShaderManager& Instance();

        VkShaderModule CreateShaderModule(const FShader::ShaderDesc& Desc, VkDevice LogicDevice);

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

        //std::unordered_set<FShader> ShaderCache;
    };
}
