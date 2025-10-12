#pragma once

#include <ylt/reflection/member_value.hpp>

#include <vector>
#include <filesystem>
#include <unordered_set>

namespace SilverBell::Renderer
{
    struct ShaderDesc
    {
        std::filesystem::path FilePath;
        std::string EntryPoint;
        std::uint32_t ShaderStage;
        std::vector<std::string> Defines;

        std::uint64_t GetHashValue() const;

        friend bool operator==(const ShaderDesc& L, const ShaderDesc& R)
        {
            return L.FilePath == R.FilePath &&
                L.EntryPoint == R.EntryPoint &&
                L.ShaderStage == R.ShaderStage &&
                L.Defines == R.Defines;
        }
    };
    YLT_REFL(ShaderDesc, FilePath, EntryPoint, ShaderStage, Defines)

    class FShader
    {
    public:
        bool HasBinary() const { return !SPIRVData.empty(); }
             
        auto GetHash() const { return BinaryHash; }

        const auto& GetReflectionInfo() const { return ReflectionInfo; }

    private:
        // 私有构造函数，通常只能允许FShaderManager创建
        explicit FShader(ShaderDesc iDesc);

        ShaderDesc Desc;
        std::vector<uint32_t> SPIRVData;
        std::uint64_t BinaryHash;

      /*  friend bool operator==(const FShader& L, const FShader& R)
        {
            return L.BinaryHash == R.BinaryHash;
        }*/

        // 这里使用位域，假定一个着色器的资源绑定点不会超过256个
        struct ShaderResourceDesc
        {
            uint32_t Set : 8;            // 描述符集编号
            uint32_t Binding : 8;        // 绑定点编号  
            uint32_t Size : 8;           // 资源数量（数组大小）
            uint32_t Stage : 8;          // 资源被用到的着色器阶段
            uint32_t Type;               // 资源类型

            friend bool operator==(const ShaderResourceDesc& L, const ShaderResourceDesc& R)
            {
                return L.Set == R.Set &&
                       L.Binding == R.Binding &&
                       L.Size == R.Size &&
                       L.Stage == R.Stage &&
                       L.Type == R.Type;
            }
        };

        struct
        {
            std::string Name;                                // 着色器名称
            uint8_t Stage;                                   // 着色器阶段
            std::vector<ShaderResourceDesc> Resources;       // 资源绑定信息
            std::unordered_set<std::string> DefinedSymbols;  // 定义的宏
            std::vector<uint32_t> InputVariables;            // 输入变量
            std::vector<uint32_t> OutputVariables;           // 输出变量
            // uint32_t LocalSizeX, LocalSizeY, LocalSizeZ;  // 计算着色器工作组大小
        } ReflectionInfo;

        friend class FShaderManager;
        friend class std::unique_ptr<FShader>;
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
