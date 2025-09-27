#include "Shader.hh"

using namespace SilverBell::Renderer;

FShader::FShader(ShaderDesc IDesc)
    : Desc(std::move(IDesc)), BinaryHash{ 0 }
{
    //BinaryHash = Algorithm::HashFunction::Hash64(SPIRVData.data(), static_cast<std::uint32_t>(SPIRVData.size() * sizeof(uint32_t)));
}
    
