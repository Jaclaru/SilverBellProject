#pragma once

#include <vector>
#include <filesystem>

#include "Hash.hh"

namespace SilverBell::Renderer
{
    class FShader
    {
    public:
        struct ShaderDesc
        {
            std::filesystem::path FilePath;
            std::string EntryPoint;
            std::uint32_t ShaderStage;
            std::vector<std::string> Defines;

            friend bool operator==(const ShaderDesc& L, const ShaderDesc& R)
            {
                return L.FilePath == R.FilePath &&
                       L.EntryPoint == R.EntryPoint &&
                       L.ShaderStage == R.ShaderStage &&
                       L.Defines == R.Defines;
            }
        };

        FShader(ShaderDesc iDesc);

        bool HasBinary() const { return !SPIRVData.empty(); }
             
        auto GetHash() const { return BinaryHash; }

    private:

        ShaderDesc Desc;
        std::vector<uint32_t> SPIRVData;
        std::uint64_t BinaryHash;

      /*  friend bool operator==(const FShader& L, const FShader& R)
        {
            return L.BinaryHash == R.BinaryHash;
        }*/

        friend class FShaderManager;
    };
}

//template<>
//struct std::hash<SilverBell::Renderer::FShader::ShaderDesc>
//{
//    std::size_t operator()(const SilverBell::Renderer::FShader::ShaderDesc& Desc) const noexcept
//    {
//        return SilverBell::Algorithm::HashFunction::HashCombine(Desc);
//    }
//};
