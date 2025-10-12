#pragma once

#include "Mixins.hh"
#include "Shader.hh"

#include <Volk/volk.h>

#include <memory>
#include <unordered_map>

namespace SilverBell::Renderer
{
    class FShaderManager : public NonCopyable
    {
    public:
        static FShaderManager& Instance();

        VkShaderModule CreateShaderModule(const ShaderDesc& Desc, VkDevice LogicDevice);

        const FShader& GetOrCreateShader(const ShaderDesc& Desc);

    private:
        FShaderManager() = default;
        ~FShaderManager() = default;

        void ReflectShader(FShader& oShader);

        std::unordered_map<uint64_t, std::unique_ptr<FShader>> ShaderCache;
    };
}
