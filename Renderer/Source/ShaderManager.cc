#include "ShaderManager.hh"

#include "RendererMarco.hh"

#include <Windows.h>
#include <wrl.h>
#include <dxc/dxcapi.h>

#include <Volk/volk.h>

#include <spdlog/spdlog.h>

#include <fstream>
#include <span>

using namespace SilverBell::Renderer;

namespace 
{
    std::string ProjectPath = PROJECT_ROOT_PATH;

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

VkShaderModule FShaderManager::CreateShaderModule(const FShader::ShaderDesc& Desc, const VkDevice LogicDevice)
{
    // TODO : 这里可以添加缓存机制，避免重复创建同一个ShaderModule

    const std::string CompletePath = ProjectPath + Desc.FilePath.string();
    auto SourceCode = ReadFile(CompletePath);

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

    std::wstring WEntyPoint(Desc.EntryPoint.begin(), Desc.EntryPoint.end());
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
    };

    auto SPIRVBinary = CompileHLSLToSPIRV(SourceCode, Argument);

    VkShaderModuleCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    CreateInfo.codeSize = SPIRVBinary.size() * sizeof(uint32_t);
    CreateInfo.pCode = reinterpret_cast<const uint32_t*>(SPIRVBinary.data());

    VkShaderModule ShaderModule;
    if (vkCreateShaderModule(LogicDevice, &CreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
    {
        spdlog::error("创建着色器模块失败，失败文件: {}", CompletePath);
        throw std::runtime_error("Failed to create shader module from file: " + CompletePath);
    }
    // 这里可以将ShaderModule存储在一个map中，以便后续使用
    // ShaderCache[ShaderType] = QString::fromStdString(FilePath); // 缓存文件路径
    // 注意：如果需要在后续使用ShaderModule时销毁它，请确保在适当的时候调用vkDestroyShaderModule
    // 例如，在渲染器清理时销毁所有ShaderModule
    // 这里可以将ShaderModule存储在一个map中，以便后续使用

    return ShaderModule;
}
