#include "ShaderManager.hh"

#include <fstream>
//#include <stdexcept>
//#include <glslang/Public/ShaderLang.h>
//#include <glslang/SPIRV/GlslangToSpv.h>

#include <iostream>
#include <Volk/volk.h>

namespace fs = std::filesystem;
using namespace SilverBell::Renderer;

namespace 
{
    std::string ProjectPath = PROJECT_ROOT_PATH;
}

namespace 
{
    std::vector<char> ReadFile(const std::string& FilePath)
    {
        // 打印当前工作目录，便于调试
        std::cout << "Current working directory: " << fs::current_path() << std::endl;


        std::ifstream File(ProjectPath + FilePath, std::ios::binary | std::ios::ate);

        if (!File.is_open())
        {
            throw std::runtime_error("Failed to open file: " + FilePath);
        }

        std::streamsize Size = File.tellg();
        std::vector<char> Buffer(Size);
        File.seekg(0);
        if (!File.read(Buffer.data(), Size))
        {
            throw std::runtime_error("Failed to read file: " + FilePath);
        }
        File.close();
        return Buffer;
    }

    VkShaderModule CreateShaderModule(VkDevice Device, const std::vector<char>& Code)
    {
        
        VkShaderModuleCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        CreateInfo.codeSize = Code.size();
        CreateInfo.pCode = reinterpret_cast<const uint32_t*>(Code.data());
        VkShaderModule ShaderModule;
        if (vkCreateShaderModule(Device, &CreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create shader module");
        }
        return ShaderModule;
    }
}

//// 添加GetDefaultResources实现
//const TBuiltInResource* GetDefaultResources()
//{
//    static const TBuiltInResource DefaultTBuiltInResource = {
//        /* .MaxLights = */ 32,
//        /* .MaxClipPlanes = */ 6,
//        /* .MaxTextureUnits = */ 32,
//        /* .MaxTextureCoords = */ 32,
//        /* .MaxVertexAttribs = */ 64,
//        /* .MaxVertexUniformComponents = */ 4096,
//        /* .MaxVaryingFloats = */ 64,
//        /* .MaxVertexTextureImageUnits = */ 32,
//        /* .MaxCombinedTextureImageUnits = */ 80,
//        /* .MaxTextureImageUnits = */ 32,
//        /* .MaxFragmentUniformComponents = */ 4096,
//        /* .MaxDrawBuffers = */ 32,
//        /* .MaxVertexUniformVectors = */ 128,
//        /* .MaxVaryingVectors = */ 8,
//        /* .MaxFragmentUniformVectors = */ 16,
//        /* .MaxVertexOutputVectors = */ 16,
//        /* .MaxFragmentInputVectors = */ 15,
//        /* .MinProgramTexelOffset = */ -8,
//        /* .MaxProgramTexelOffset = */ 7,
//        /* .MaxClipDistances = */ 8,
//        /* .MaxComputeWorkGroupCountX = */ 65535,
//        /* .MaxComputeWorkGroupCountY = */ 65535,
//        /* .MaxComputeWorkGroupCountZ = */ 65535,
//        /* .MaxComputeWorkGroupSizeX = */ 1024,
//        /* .MaxComputeWorkGroupSizeY = */ 1024,
//        /* .MaxComputeWorkGroupSizeZ = */ 64,
//        /* .MaxComputeUniformComponents = */ 1024,
//        /* .MaxComputeTextureImageUnits = */ 16,
//        /* .MaxComputeImageUniforms = */ 8,
//        /* .MaxComputeAtomicCounters = */ 8,
//        /* .MaxComputeAtomicCounterBuffers = */ 1,
//        /* .MaxVaryingComponents = */ 60,
//        /* .MaxVertexOutputComponents = */ 64,
//        /* .MaxGeometryInputComponents = */ 64,
//        /* .MaxGeometryOutputComponents = */ 128,
//        /* .MaxFragmentInputComponents = */ 128,
//        /* .MaxImageUnits = */ 8,
//        /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
//        /* .MaxCombinedShaderOutputResources = */ 8,
//        /* .MaxImageSamples = */ 0,
//        /* .MaxVertexImageUniforms = */ 0,
//        /* .MaxTessControlImageUniforms = */ 0,
//        /* .MaxTessEvaluationImageUniforms = */ 0,
//        /* .MaxGeometryImageUniforms = */ 0,
//        /* .MaxFragmentImageUniforms = */ 8,
//        /* .MaxCombinedImageUniforms = */ 8,
//        /* .MaxGeometryTextureImageUnits = */ 16,
//        /* .MaxGeometryOutputVertices = */ 256,
//        /* .MaxGeometryTotalOutputComponents = */ 1024,
//        /* .MaxGeometryUniformComponents = */ 1024,
//        /* .MaxGeometryVaryingComponents = */ 64,
//        /* .MaxTessControlInputComponents = */ 128,
//        /* .MaxTessControlOutputComponents = */ 128,
//        /* .MaxTessControlTextureImageUnits = */ 16,
//        /* .MaxTessControlUniformComponents = */ 1024,
//        /* .MaxTessControlTotalOutputComponents = */ 4096,
//        /* .MaxTessEvaluationInputComponents = */ 128,
//        /* .MaxTessEvaluationOutputComponents = */ 128,
//        /* .MaxTessEvaluationTextureImageUnits = */ 16,
//        /* .MaxTessEvaluationUniformComponents = */ 1024,
//        /* .MaxTessPatchComponents = */ 120,
//        /* .MaxPatchVertices = */ 32,
//        /* .MaxTessGenLevel = */ 64,
//        /* .MaxViewports = */ 16,
//        /* .MaxVertexAtomicCounters = */ 0,
//        /* .MaxTessControlAtomicCounters = */ 0,
//        /* .MaxTessEvaluationAtomicCounters = */ 0,
//        /* .MaxGeometryAtomicCounters = */ 0,
//        /* .MaxFragmentAtomicCounters = */ 8,
//        /* .MaxCombinedAtomicCounters = */ 8,
//        /* .MaxAtomicCounterBindings = */ 1,
//        /* .MaxVertexAtomicCounterBuffers = */ 0,
//        /* .MaxTessControlAtomicCounterBuffers = */ 0,
//        /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
//        /* .MaxGeometryAtomicCounterBuffers = */ 0,
//        /* .MaxFragmentAtomicCounterBuffers = */ 1,
//        /* .MaxCombinedAtomicCounterBuffers = */ 1,
//        /* .MaxAtomicCounterBufferSize = */ 16384,
//        /* .MaxTransformFeedbackBuffers = */ 4,
//        /* .MaxTransformFeedbackInterleavedComponents = */ 64,
//        /* .MaxCullDistances = */ 8,
//        /* .MaxCombinedClipAndCullDistances = */ 8,
//        /* .MaxSamples = */ 4,
//        ///* .limits = */ {
//        //    /* .nonInductiveForLoops = */ 1,
//        //    /* .whileLoops = */ 1,
//        //    /* .doWhileLoops = */ 1,
//        //    /* .generalUniformIndexing = */ 1,
//        //    /* .generalAttributeMatrixVectorIndexing = */ 1,
//        //    /* .generalVaryingIndexing = */ 1,
//        //    /* .generalSamplerIndexing = */ 1,
//        //    /* .generalVariableIndexing = */ 1,
//        //    /* .generalConstantMatrixVectorIndexing = */ 1,
//        //}
//    };
//    return &DefaultTBuiltInResource;
//}
//
//std::vector<uint32_t> CompileWithGlslang(
//    const std::string& hlslSource,
//    EShLanguage stage)
//{
//    // 初始化glslang
//    glslang::InitializeProcess();
//
//    // 创建着色器对象
//    glslang::TShader shader(stage);
//    const char* src = hlslSource.c_str();
//    shader.setStrings(&src, 1);
//
//    // 设置HLSL输入
//    shader.setEnvInput(glslang::EShSourceHlsl, stage, glslang::EShClientVulkan, 100);
//    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
//    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
//
//    // 编译
//    if (!shader.parse(GetDefaultResources(), 100, false, EShMsgDefault)) {
//        throw std::runtime_error(shader.getInfoLog());
//    }
//
//    // 链接为SPIR-V
//    glslang::TProgram program;
//    program.addShader(&shader);
//    if (!program.link(EShMsgDefault)) {
//        throw std::runtime_error(program.getInfoLog());
//    }
//
//    // 生成SPIR-V
//    std::vector<uint32_t> spirv;
//    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);
//
//    glslang::FinalizeProcess();
//    return spirv;
//}

FShaderManager& FShaderManager::Instance()
{
    static FShaderManager sInstance;
    return sInstance;
}

void FShaderManager::CompileHLSL2SPIRV(const std::string& FilePath, const std::string& EntryPoint, const std::string& ShaderModel, uint32_t ShaderType)
{
}

VkShaderModule FShaderManager::CreateShaderModule(const std::string& FilePath, const VkDevice LogicDevice)
{
    // TODO : 这里可以添加缓存机制，避免重复创建同一个ShaderModule
    //auto finder = ShaderCache.find(LogicDevice);
    //if (finder == ShaderCache.end())
    //{
    //    ShaderCache[LogicDevice] = ShaderModuleCache{ LogicDevice };
    //    return finder->second; // 如果缓存中存在，直接返回
    //}

    auto ShaderCode = ReadFile(FilePath);

    VkShaderModuleCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    CreateInfo.codeSize = ShaderCode.size();
    CreateInfo.pCode = reinterpret_cast<const uint32_t*>(ShaderCode.data());

    VkShaderModule ShaderModule;
    if (vkCreateShaderModule(LogicDevice, &CreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module from file: " + FilePath);
    }
    // 这里可以将ShaderModule存储在一个map中，以便后续使用
    // ShaderCache[ShaderType] = QString::fromStdString(FilePath); // 缓存文件路径
    // 注意：如果需要在后续使用ShaderModule时销毁它，请确保在适当的时候调用vkDestroyShaderModule
    // 例如，在渲染器清理时销毁所有ShaderModule
    // 这里可以将ShaderModule存储在一个map中，以便后续使用


    return ShaderModule;
}
