#include "ShaderManager.hh"

#include "RendererMarco.hh"

#include <Windows.h>
#include <wrl.h>
#include <dxc/dxcapi.h>

#include <Volk/volk.h>

#include <spdlog/spdlog.h>

#include <fstream>
#include <span>

#include "spirv_reflect.h"

using namespace SilverBell::Renderer;

namespace 
{
    const std::string ProjectPath = PROJECT_ROOT_PATH;

    template<typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::vector<char> ReadFile(const std::string& FilePath)
    {
        // 打印当前工作目录，便于调试
        //std::cout << "Current working directory: " << fs::current_path() << std::endl;

        std::ifstream File(FilePath, std::ios::binary | std::ios::ate);

        if (!File.is_open())
        {
            spdlog::error("打开文件失败: {}", FilePath);
            throw std::runtime_error("Failed to open file: " + FilePath);
        }

        std::streamsize Size = File.tellg();
        std::vector<char> Buffer(Size);
        File.seekg(0);
        if (!File.read(Buffer.data(), Size))
        {
            spdlog::error("读入文件失败: {}", FilePath);
            throw std::runtime_error("Failed to read file: " + FilePath);
        }
        File.close();
        return Buffer;
    }

    constexpr uint8_t MAX_SHADER_INPUT = 8;
}

FShaderManager& FShaderManager::Instance()
{
    static FShaderManager sInstance;
    return sInstance;
}

std::vector<uint32_t> CompileHLSLToSPIRV(const std::span<char> SourceCode, std::span<const wchar_t*> Arguments)
{
    // 初始化DXCs
    static ComPtr<IDxcUtils> IDxcUtils = nullptr;
    static ComPtr<IDxcCompiler3> Compiler = nullptr;
    static ComPtr<IDxcIncludeHandler> IncludeHandler = nullptr;
    
    if (Compiler == nullptr)
    {
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&IDxcUtils));
        DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler));
        IDxcUtils->CreateDefaultIncludeHandler(&IncludeHandler);
    }

    ComPtr<IDxcBlobEncoding> SourceBlob;
    IDxcUtils->CreateBlob(SourceCode.data(), static_cast<UINT32>(SourceCode.size()), CP_UTF8, &SourceBlob);
    DxcBuffer SourceBuffer = { .Ptr = SourceBlob->GetBufferPointer(), .Size = SourceBlob->GetBufferSize(), .Encoding = DXC_CP_UTF8 };
    // 编译
    ComPtr<IDxcResult> Results;
    Compiler->Compile(
        &SourceBuffer,
        Arguments.data(),
        static_cast<UINT32>(Arguments.size()),
        IncludeHandler.Get(),
        IID_PPV_ARGS(&Results));


    // 检查编译状态
    HRESULT Status;
    Results->GetStatus(&Status);

    if (FAILED(Status)) 
    {
        ComPtr<IDxcBlobUtf8> Errors;
        Results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&Errors), nullptr);
        spdlog::error("HLSL编译失败: {}", Errors->GetStringPointer());
        throw std::runtime_error(Errors->GetStringPointer());
    }

    // 获取 SPIR-V 二进制
    ComPtr<IDxcBlob> SpirvBlob;
    Results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&SpirvBlob), nullptr);

    // 转换为 uint32_t 数组
    const uint32_t* SpirvData = reinterpret_cast<const uint32_t*>(SpirvBlob->GetBufferPointer());
    size_t SpirvSize = SpirvBlob->GetBufferSize() / sizeof(uint32_t);

    return { SpirvData, SpirvData + SpirvSize };
}

VkShaderModule FShaderManager::CreateShaderModule(const ShaderDesc& Desc, const VkDevice LogicDevice)
{
    // TODO : 这里可以添加缓存机制，避免重复创建同一个ShaderModule

    const auto& Shader = GetOrCreateShader(Desc);

    auto& SPIRVBinary = Shader.SPIRVData;
    VkShaderModuleCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    CreateInfo.codeSize = SPIRVBinary.size() * sizeof(uint32_t);
    CreateInfo.pCode = (SPIRVBinary.data());

    VkShaderModule ShaderModule;
    if (vkCreateShaderModule(LogicDevice, &CreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
    {
        spdlog::error("创建着色器模块失败，失败文件: {}", Desc.FilePath.string());
        throw std::runtime_error("Failed to create shader module from file: " + Desc.FilePath.string());
    }

    return ShaderModule;
}

const FShader& FShaderManager::GetOrCreateShader(const ShaderDesc& Desc)
{
    auto ShaderHash = Desc.GetHashValue();
    if (ShaderCache.contains(ShaderHash)) return *ShaderCache[ShaderHash];

    // 读取着色器文件
    const std::string CompletePath = ProjectPath + Desc.FilePath.string();
    auto SourceCode = ReadFile(CompletePath);

    // 文件名
    std::string FileName = Desc.FilePath.filename().string();
    std::wstring WFileName(FileName.begin(), FileName.end());

    // 着色器模型字符串
    std::wstring WShaderModel;
    switch (Desc.ShaderStage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT:
        WShaderModel = L"vs_6_0";
        break;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        WShaderModel = L"ps_6_0";
        break;
    default:
        spdlog::error("错误的着色器阶段！");
    }

    // 入口点
    std::wstring WEntyPoint(Desc.EntryPoint.begin(), Desc.EntryPoint.end());

    // 装配编译参数
    std::vector Argument =
    {
        L"-E", WEntyPoint.c_str(),
        L"-T", WShaderModel.c_str(),
        L"-spirv",
        L"-fvk-use-dx-layout", // 使用DirectX的内存布局
        L"-fspv-target-env=vulkan1.1", // TODO:做成可配置的
#if VULKAN_DEBUG_ENABLE
        L"-Zi",
#endif
        WFileName.c_str(),
    };

    // 编译成SPIR-V二进制
    auto SPIRVBinary = CompileHLSLToSPIRV(SourceCode, Argument);

    auto Shader = std::unique_ptr<FShader>(new FShader(Desc));
    Shader->SPIRVData = std::move(SPIRVBinary);

    SpvReflectShaderModule Module;
    SpvReflectResult Result = spvReflectCreateShaderModule(Shader->SPIRVData.size() * sizeof(uint32_t), Shader->SPIRVData.data(), &Module);
    if (Result != SPV_REFLECT_RESULT_SUCCESS)
    {
        spdlog::error("SPIR-V反射创建失败，失败文件: {}", CompletePath);
        throw std::runtime_error("Failed to reflect shader module from file: " + CompletePath);
    }

    // 注：反射库中的枚举类型与Vulkan的枚举类型是一致的，可以直接使用

    // 获取入口点信息
    const SpvReflectEntryPoint* EntryPoint = spvReflectGetEntryPoint(&Module, Desc.EntryPoint.c_str());
    // 名称采用 文件名-入口点名 的形式，便于区分同一文件中的不同入口点
    Shader->ReflectionInfo.Name = FileName + std::string("-") + std::string(EntryPoint->name);
    Shader->ReflectionInfo.Stage = static_cast<uint8_t>(EntryPoint->shader_stage);
    // 获取输入变量信息
    uint32_t InputVariableCount = 0;
    spvReflectEnumerateInputVariables(&Module, &InputVariableCount, nullptr);
    if (InputVariableCount)
    {
        std::vector<SpvReflectInterfaceVariable*> InputVariables(InputVariableCount);
        spvReflectEnumerateInputVariables(&Module, &InputVariableCount, InputVariables.data());
        for (const auto* Var : InputVariables)
        {
            if (Var->location < MAX_SHADER_INPUT)
            {
                if (Shader->ReflectionInfo.InputVariables.size() < Var->location + 1)
                {
                    Shader->ReflectionInfo.InputVariables.resize(Var->location + 1);
                }
                Shader->ReflectionInfo.InputVariables[Var->location] = Var->format;
            }
        }
    }

    // 获取输出变量信息
    uint32_t OutputVariableCount = 0;
    spvReflectEnumerateOutputVariables(&Module, &OutputVariableCount, nullptr);
    if (OutputVariableCount)
    {
        std::vector<SpvReflectInterfaceVariable*> OutputVariables(OutputVariableCount);
        spvReflectEnumerateOutputVariables(&Module, &OutputVariableCount, OutputVariables.data());
        for (const auto* Var : OutputVariables)
        {
            if (Var->location < MAX_SHADER_INPUT)
            {
                if (Shader->ReflectionInfo.OutputVariables.size() < Var->location + 1)
                {
                    Shader->ReflectionInfo.OutputVariables.resize(Var->location + 1);
                }
                Shader->ReflectionInfo.OutputVariables[Var->location] = Var->format;
            }
        }
    }

    // 获取描述符集信息
    uint32_t DescriptorSetCount = 0;
    spvReflectEnumerateDescriptorSets(&Module, &DescriptorSetCount, nullptr);
    if (DescriptorSetCount)
    {
        std::vector<SpvReflectDescriptorSet*> DescriptorSets(DescriptorSetCount);
        spvReflectEnumerateDescriptorSets(&Module, &DescriptorSetCount, DescriptorSets.data());

        for (const auto* Set : DescriptorSets)
        {
            for (uint32_t Idx = 0; Idx < Set->binding_count; ++Idx)
            {
                const auto& Binding = Set->bindings[Idx];
                FShader::ShaderResourceDesc ResourceDesc;
                ResourceDesc.Set = static_cast<uint8_t>(Set->set);
                ResourceDesc.Binding = static_cast<uint8_t>(Binding->binding);
                ResourceDesc.Size = static_cast<uint8_t>(Binding->count);
                ResourceDesc.Stage = static_cast<uint8_t>(EntryPoint->shader_stage);
                ResourceDesc.Type = static_cast<uint32_t>(Binding->descriptor_type);
                Shader->ReflectionInfo.Resources.push_back(ResourceDesc);
            }
        }
    }

    // 存入缓存
    ShaderCache[ShaderHash] = std::move(Shader);
    return *ShaderCache[ShaderHash];
}

void FShaderManager::ReflectShader(FShader& oShader)
{

}
